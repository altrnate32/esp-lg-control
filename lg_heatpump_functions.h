    //static const char* state_string_friendly[] = {"Initialiseren","Uit","Start","Opstarten","Aan (stabiliseren)","Aan (verwarmen)","Aan (overshoot)","Aan (stall)","Pauze (Uit)","Aan (Warm Water)","Ontdooien","Nadraaien"};
    //static const char* state_string[] = {"INIT","IDLE","START","STARTING","STABILIZE","RUN","OVERSHOOT","STALL","WAIT","SWW","DEFROST","AFTERRUN"};
    enum states {NONE,INIT,IDLE,START,STARTING,STABILIZE,RUN,OVERSHOOT,STALL,WAIT,SWW,DEFROST,AFTERRUN};
    enum input_types {THERMOSTAT,THERMOSTAT_SENSOR,COMPRESSOR,SWW_RUN,DEFROST_RUN,OAT,STOOKLIJN_TARGET,TRACKING_VALUE,BOOST,BACKUP_HEAT,EXTERNAL_PUMP,RELAY_HEAT,TEMP_NEW_TARGET};
    struct input_struct {
      bool state = false;
      bool prev_state = false;
      float value = 0.0;
      float prev_value = 0.0;
      uint_fast32_t input_change_time;
      void receive_state(bool new_state, uint_fast32_t run_time){
          state = new_state;
          if(new_state != prev_state) input_change_time = run_time;
      }
      void receive_value(float new_value, uint_fast32_t run_time){
          value = new_value;
          if(new_value != prev_value) input_change_time = run_time;
      }
      bool has_flag(){
        return (prev_value != value || prev_state != state);
      }
      void unflag(){
        prev_state = state;
        prev_value = value;
      }
    };
   
    class state_machine_class {
      private:
        states current_state = INIT; //current state the machine is in
        states prev_state = NONE; //previous state
        states next_state = NONE; //next state (in case of state change)
        
        std::vector<input_types> events; //vector with events (changed inputs)
        uint_fast32_t run_time_value = 0; //total esp boot time
        uint_fast32_t state_start_time = 0; //run_time_value on last state change
        uint_fast32_t run_start_time = 0; //run_time_value of start of heat run
        int current_boost_offset = 0 ;//keep track of offset during boost mode. Will be 0 if boost is not active
        static std::vector<float> derivative; //vector of floats to integrate derivative (used in control logic)
        static std::vector<float> cop_vector; //vector of floats to integrate cop
        float delta = 0; //Current Error value negative below target, positive above target
        float pendel_delta = 0; //Error value in regard to pendel target
        float average_cop = 0; //keep track off average cop during last 15 minutes
        bool backup_heat_cop_limit_trigger = false; //if backup heat triggered due to low cop?
      public:
        input_struct* input[11]; //list of all inputs
        bool entry_done = false;
        bool exit_done = false;
        //default values, change these if you want
        int boost_offset = 2; //number of degrees to raise stooklijn in boost mode
        int hysteresis = 4; //Set controller control mode to 'outlet' and set hysteresis to the setting you have on the controller (recommend 4)
        int max_overshoot = 3; //maximum allowable overshoot in 'OVERSHOOT' state
        int alive_timer = 120; //interval in seconds for an 'alive' message in the logs
        float backup_heater_cop_limit = 1.5; //below which cop should the backup heater be switched on? Set to a large negative number (example -1000) to disable 
        float derivative_D_5 = 0; //derivative based on past 5 minutes
        float derivative_D_10 = 0; //derivative based on past 10 minutes
        float pred_20_delta_5 = 0; //predicted delta in 20 minutes based on last 5 minute derivative
        float pred_20_delta_10 = 0; //predicted delta in 20 minutes based on last 10 minute derivative
        float pred_5_delta_5 = 0; //predicted delta in 5 minutes based on last 5 minute derivative
        state_machine_class(){
            //initialize the list with inputs
            input[THERMOSTAT] = new input_struct();
            input[THERMOSTAT_SENSOR] = new input_struct();
            input[COMPRESSOR] = new input_struct();
            input[SWW_RUN] = new input_struct();
            input[DEFROST_RUN] = new input_struct();
            input[OAT] = new input_struct();
            input[STOOKLIJN_TARGET] = new input_struct();
            input[TRACKING_VALUE] = new input_struct();
            input[BOOST] = new input_struct();
            input[BACKUP_HEAT] = new input_struct();
            input[EXTERNAL_PUMP] = new input_struct();
            input[RELAY_HEAT] = new input_struct();
            input[TEMP_NEW_TARGET] = new input_struct();
        }
        ~ state_machine_class(){
            delete input[THERMOSTAT];
            delete input[THERMOSTAT_SENSOR];
            delete input[COMPRESSOR];
            delete input[SWW_RUN];
            delete input[DEFROST_RUN];
            delete input[OAT];
            delete input[STOOKLIJN_TARGET];
            delete input[TRACKING_VALUE];
            delete input[BOOST];
            delete input[BACKUP_HEAT];
            delete input[EXTERNAL_PUMP];
            delete input[RELAY_HEAT];
            delete input[TEMP_NEW_TARGET];
        }
        states state(){
            return current_state;
        }
        states get_prev_state(){
            return prev_state;
        }
        states get_next_state(){
            return next_state;
        }
        void state_transition(states newstate){
            next_state = newstate;
            ESP_LOGD(state_name().c_str(), "State transition-> %s",state_name(get_next_state()).c_str());
        }
        void handle_state_transition(){
          if(next_state != current_state){
            prev_state = current_state;
            current_state = get_next_state();
            state_start_time = get_run_time();
          }
        }
        std::string state_friendly_name(states stt = NONE){
          if(stt == NONE) stt = current_state;
          static const std::string state_string_friendly_list[13] = {"Null","Initialiseren","Uit","Start","Opstarten","Aan (stabiliseren)","Aan (verwarmen)","Aan (overshoot)","Aan (stall)","Pauze (Uit)","Aan (Warm Water)","Ontdooien","Nadraaien"}; 
          return state_string_friendly_list[stt];
        }
        std::string state_name(states stt = NONE){
          if(stt == NONE) stt = current_state;
          static const std::string state_string_list[13] = {"NULL","INIT","IDLE","START","STARTING","STABILIZE","RUN","OVERSHOOT","STALL","WAIT","SWW","DEFROST","AFTERRUN"};
          return state_string_list[stt];
        }
        uint_fast32_t get_run_time(){
          return run_time_value;
        }
        void increment_run_time(uint_fast32_t increment){
            run_time_value += increment;
        }
        void set_run_start_time(uint_fast32_t time){
            run_start_time = time;
        }
        uint_fast32_t get_run_start_time(){
            return run_start_time;
        }
        bool is_compressor_modulating(){
            //TODO
            return false;
        }
        //receive all values, booleans (states) or floats (values)
        void receive_inputs(){
          input[THERMOSTAT_SENSOR]->receive_state(id(thermostat_signal).state,get_run_time()); //state of thermostat input
          if(input[THERMOSTAT_SENSOR]->has_flag()) input[THERMOSTAT]->receive_state(thermostat_state(),get_run_time());
          input[COMPRESSOR]->receive_state(id(compressor_running).state,get_run_time()); //is the compressor running
          input[SWW_RUN]->receive_state(id(sww_heating).state,get_run_time()); //is the domestic hot water run active
          input[DEFROST_RUN]->receive_state(id(defrosting).state,get_run_time()); //is defrost active
          input[OAT]->receive_value(round(id(buiten_temp).state),get_run_time()); //outside air temperature
          if(input[OAT]->has_flag() || id(update_stooklijn)) input[STOOKLIJN_TARGET]->receive_value(calculate_stooklijn(),get_run_time()); //stooklijn target
          //Set to value that anti-pendel script will track (outlet/inlet) (recommend inlet)
          input[TRACKING_VALUE]->receive_value(floor(id(water_temp_aanvoer).state),get_run_time());
          //input[BOOST] internal input, we dont receive a value here
          input[BACKUP_HEAT]->receive_state(id(relay_backup_heat).state,get_run_time()); //is backup heat on/off
          input[EXTERNAL_PUMP]->receive_state(id(relay_pump).state,get_run_time()); //is external pump on/off
          input[RELAY_HEAT]->receive_state(id(relay_heat).state,get_run_time()); //is realy_heat (heatpump external thermostat contact) on/off
          if(input[TEMP_NEW_TARGET]->value == 0.0) input[TEMP_NEW_TARGET]->value = input[STOOKLIJN_TARGET]->value;
          delta = input[TRACKING_VALUE]->value-input[STOOKLIJN_TARGET]->value; 
          pendel_delta = input[TRACKING_VALUE]->value-input[TEMP_NEW_TARGET]->value;
        }
        //set input.value.prev_value = input_value.value to remove the implicit 'value changed' flag
        void unflag_input_values(){
          input[THERMOSTAT_SENSOR]->unflag();
          input[THERMOSTAT]->unflag();
          input[COMPRESSOR]->unflag();
          input[SWW_RUN]->unflag();
          input[DEFROST_RUN]->unflag();
          input[OAT]->unflag();
          input[STOOKLIJN_TARGET]->unflag();
          input[TRACKING_VALUE]->unflag();
          input[BOOST]->unflag();
          input[BACKUP_HEAT]->unflag();
          input[EXTERNAL_PUMP]->unflag();
          input[RELAY_HEAT]->unflag();
          input[TEMP_NEW_TARGET]->unflag();
        }
        //calculate stooklijn function
        float calculate_stooklijn(){
          //Calculate stooklijn target
          //Hold previous script run oat value
          static float prev_oat = 20; //oat at minimum water temp (20/20) to prevent strange events on startup
          //wait for a valid oat reading
          float oat = 20;
          if(input[OAT]->value > 60 || input[OAT]->value < -50 || isnan(input[OAT]->value)){
            oat = prev_oat;
            //use prev_oat (or 20) and do not set update_stooklijn to false, to trigger a new run on next cycle
            ESP_LOGD("calculate_stooklijn", "Invalid OAT (%f) waiting for next run", input[OAT]->value);
          }  else {
            prev_oat = input[OAT]->value;
            id(update_stooklijn) = false;
          }
          //OAT expects start temp to be OAT 20 with Watertemp 20. Steepness is defined bij Z, calculated by the max wTemp at minOat
          //Formula is wTemp = ((Z x (20 - OAT)) + 20) + C 
          //Formula to calculate Z = 0-((stooklijn_max_wtemp-20)) / (stooklijn_min_oat - stooklijn_start_temp))
          //C is the curvature of the stooklijn defined by C = (stooklijn_curve*0.001)*(oat-20)^2
          //This will add a positive offset with decreasing offset. You can set this to zero if you don't need it and want a linear stooklijn
          //I need it in my installation as the stooklijn is spot on at relative high temperatures, but too low at lower temps
          const float Z =  0 - (float)( (id(stooklijn_max_wtemp).state-20)/(id(stooklijn_min_oat).state - 20));
          //If oat below minimum oat, clamp to minimum value
          clamp(input[OAT]->value,id(stooklijn_min_oat).state,(float)20.0);
          float C = (id(stooklijn_curve).state*0.001)*pow((input[OAT]->value-20),2);
          id(stooklijn_target) = (int)round( (Z * (20-input[OAT]->value))+20+C);
          //Add stooklijn offset
          id(stooklijn_target) = id(stooklijn_target) + id(wp_stooklijn_offset).state;
          //Add boost offset
           id(stooklijn_target) = id(stooklijn_target) + current_boost_offset;
          //Clamp target to minimum temp/max water+3
          clamp(id(stooklijn_target),id(stooklijn_min_wtemp).state,id(stooklijn_max_wtemp).state+3);
          ESP_LOGD("calculate_stooklijn", "Stooklijn calculated with oat: %f, Z: %f, C: %f offset: %f, result: %f",input[OAT]->value, Z, C, id(wp_stooklijn_offset).state, id(stooklijn_target));
          //Publish new stooklijn value to watertemp value sensor
          id(watertemp_target).publish_state(id(stooklijn_target));
          return id(stooklijn_target);
        }
        //handle on/off delay to publish TERMOSTAT state
        bool thermostat_state(){
          //instant on
          if(state() == INIT && input[THERMOSTAT_SENSOR]->state) return true;
          //if no change, return
          if(input[THERMOSTAT_SENSOR]->state == input[THERMOSTAT]->state) return input[THERMOSTAT]->state; 
          //thermostat change
          if(input[THERMOSTAT_SENSOR]->state){
            //state change is a switch to on
            //check if on delay has passed
            if((get_run_time() - input[THERMOSTAT_SENSOR]->input_change_time) > (id(thermostat_on_delay).state*60)) return true;
          } else {
            //state change is a switch to off
            //check for instant off
            if(!input[COMPRESSOR]->state || state() == SWW || state() == DEFROST) return false;
            //check if off delay time has passed
            if((get_run_time() - input[THERMOSTAT_SENSOR]->input_change_time) > (id(thermostat_off_delay).state*60)) {
              //then check if minimum run time has passed
              if((get_run_time() - run_start_time) > (id(minimum_run_time).state * 60))return false;
            }
          }
          //return previous state
          return input[THERMOSTAT]->state;
        }
    };

    //struct state {
    //  enum state {INIT,IDLE,START,STARTING,STABILIZE,RUN,OVERSHOOT,STALL,WAIT,SWW,DEFROST,AFTERRUN};
    //  static const char* state_string_friendly[] = {"Initialiseren","Uit","Start","Opstarten","Aan (stabiliseren)","Aan (verwarmen)","Aan (overshoot)","Aan (stall),"Pauze (Uit)","Aan (Warm Water)","Ontdooien","Nadraaien"};
    //  static const char* state_string[] = {"INIT","IDLE","START","STARTING","STABILIZE","RUN","OVERSHOOT","STALL","WAIT","SWW","DEFROST","AFTERRUN"};
    //  uint_fast32_t state_start_time;
    //}

    //update target temp through modbus
    void set_target_temp(float target){
      auto water_temp_call = id(water_temp_target_output).make_call();
      water_temp_call.set_value(round(target));
      water_temp_call.perform();
      ESP_LOGD("set_target_temp", "Modbus target set to: %f", round(target));
    }
    
    void calculate_derivative(float tracking_value,std::vector<float>* derivative, float* derivative_D_5,float* derivative_D_10){
      //calculate derivative
      float d_number = tracking_value;
      derivative->push_back(d_number);
      //limit size to 31 elements(15 minutes)
      if(derivative->size() > 31) derivative->erase(derivative->begin());
      //calculate current derivative for 5 and 10 minutes
      //derivative is measured in degrees/minute
      *derivative_D_5 = 0;
      *derivative_D_10 = 0;
      //wait until derivative > 14, this makes sure the first 2 minutes are skipped
      //first minute or so is unreliabel if pump has been off for a while (water cools in the unit)
      if(derivative->size() > 14){   
        *derivative_D_5 = (derivative->back()-derivative->at(derivative->size()-11))/10;
      }
      if(derivative->size() > 24){   
      *derivative_D_10 = (derivative->back()-derivative->at(derivative->size()-21))/20;
      id(derivative_value).publish_state(*derivative_D_10*60);
     }
    }
  
    
