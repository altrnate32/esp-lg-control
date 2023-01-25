  enum states {NONE,INIT,IDLE,START,STARTING,STABILIZE,RUN,OVERSHOOT,STALL,WAIT,SWW,DEFROST,AFTERRUN};
  enum input_types {THERMOSTAT,THERMOSTAT_SENSOR,COMPRESSOR,SWW_RUN,DEFROST_RUN,OAT,STOOKLIJN_TARGET,TRACKING_VALUE,BOOST,BACKUP_HEAT,EXTERNAL_PUMP,RELAY_HEAT,TEMP_NEW_TARGET,WP_PUMP,SILENT_MODE,EMERGENCY};
  struct input_struct {
    bool state = false;
    bool prev_state = false;
    float value = 0.0;
    float prev_value = 0.0;
    uint_fast32_t input_change_time;
    uint_fast32_t* run_time;
    input_struct(uint_fast32_t* run_time_pointer);
    void receive_state(bool new_state);
    void receive_value(float new_value);
    uint_fast32_t seconds_since_change();
    bool has_flag();
    void unflag();
  };

    class state_machine_class {
      private:
        states current_state = INIT; //current state the machine is in
        states prev_state = NONE; //previous state
        states next_state = NONE; //next state (in case of state change)
        std::vector<input_types> events; //vector with events to check
        uint_fast32_t run_time_value = 0; //total esp boot time
        uint_fast32_t state_start_time = 0; //run_time_value on last state change
        uint_fast32_t run_start_time = 0; //run_time_value of start of heat run
        int current_boost_offset = 0 ;//keep track of offset during boost mode. Will be 0 if boost is not active
        std::vector<float> derivative; //vector of floats to integrate derivative (used in control logic)
        bool backup_heat_temp_limit_trigger = false; //if backup heat triggered due to low temperature (always on)?
        bool update_stooklijn_bool = true;
      public:
        input_struct* input[16]; //list of all inputs
        bool entry_done = false;
        //default values, change these if you want
        int boost_offset = 2; //number of degrees to raise stooklijn in boost mode
        int hysteresis = 4; //Set controller control mode to 'outlet' and set hysteresis to the setting you have on the controller (recommend 4)
        int max_overshoot = 3; //maximum allowable overshoot in 'OVERSHOOT' state
        int alive_timer = 120; //interval in seconds for an 'alive' message in the logs
        float delta = 0; //Current Error value negative below target, positive above target
        float pendel_delta = 0; //Error value in regard to pendel target
        float derivative_D_5 = 0; //derivative based on past 5 minutes
        float derivative_D_10 = 0; //derivative based on past 10 minutes
        float pred_20_delta_5 = 0; //predicted delta in 20 minutes based on last 5 minute derivative
        float pred_20_delta_10 = 0; //predicted delta in 20 minutes based on last 10 minute derivative
        float pred_5_delta_5 = 0; //predicted delta in 5 minutes based on last 5 minute derivative
        state_machine_class();
        ~ state_machine_class();
        void update_stooklijn();
        states state();
        states get_prev_state();
        states get_next_state();
        void state_transition(states newstate);
        void handle_state_transition();
        const char * state_friendly_name(states stt = NONE);
        const char * state_name(states stt = NONE);
        uint_fast32_t get_run_time();
        void increment_run_time(uint_fast32_t increment);
        void set_run_start_time();
        uint_fast32_t get_run_start_time();
        uint_fast32_t get_state_start_time();
        uint_fast32_t seconds_since_state_start();
        void receive_inputs();
        void process_inputs();
        void unflag_input_values();
        float calculate_stooklijn();
        bool thermostat_state();
        void calculate_derivative(float tracking_value);
        void heat(bool mode);
        void external_pump(bool mode);
        void backup_heat(bool mode,bool temp_limit_trigger = false);
        void boost(bool mode);
        void toggle_boost();
        void silent_mode(bool mode);
        void toggle_silent_mode();
        int get_target_offset();
        void set_new_target(float new_target);
        void start_events();
        void add_event(input_types ev);
        bool check_change_events();
        bool compressor_modulation();
        bool check_low_temp_trigger();
        void set_target_temp(float target);
    };
  input_struct::input_struct(uint_fast32_t* run_time_pointer){
    run_time = run_time_pointer;
  }
  void input_struct::receive_state(bool new_state){
    state = new_state;
    if(new_state != prev_state)  input_change_time = *run_time;
  }
  void input_struct::receive_value(float new_value){
    value = new_value;
    if(new_value != prev_value) input_change_time = *run_time;
  }
  uint_fast32_t input_struct::seconds_since_change(){
    return *run_time - input_change_time;
  }
  bool input_struct::has_flag(){
    return (prev_value != value || prev_state != state);
  }
  void input_struct::unflag(){
    prev_state = state;
    prev_value = value;
  }
  state_machine_class::state_machine_class(){
    //initialize the list with inputs
    input[THERMOSTAT] = new input_struct(&run_time_value);
    input[THERMOSTAT_SENSOR] = new input_struct(&run_time_value);
    input[COMPRESSOR] = new input_struct(&run_time_value);
    input[SWW_RUN] = new input_struct(&run_time_value);
    input[DEFROST_RUN] = new input_struct(&run_time_value);
    input[OAT] = new input_struct(&run_time_value);
    input[STOOKLIJN_TARGET] = new input_struct(&run_time_value);
    input[TRACKING_VALUE] = new input_struct(&run_time_value);
    input[BOOST] = new input_struct(&run_time_value);
    input[BACKUP_HEAT] = new input_struct(&run_time_value);
    input[EXTERNAL_PUMP] = new input_struct(&run_time_value);
    input[RELAY_HEAT] = new input_struct(&run_time_value);
    input[TEMP_NEW_TARGET] = new input_struct(&run_time_value);
    input[WP_PUMP] = new input_struct(&run_time_value);
    input[SILENT_MODE] = new input_struct(&run_time_value);
    input[EMERGENCY] = new input_struct(&run_time_value);
  }
  state_machine_class::~ state_machine_class(){
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
    delete input[WP_PUMP];
    delete input[SILENT_MODE];
    delete input[EMERGENCY];
  }
  void state_machine_class::update_stooklijn(){
    update_stooklijn_bool = true;
  }
  states state_machine_class::state(){
    return current_state;
  }
  states state_machine_class::get_prev_state(){
    return prev_state;
  }
  states state_machine_class::get_next_state(){
    return next_state;
  }
  void state_machine_class::state_transition(states newstate){
    next_state = newstate;
    ESP_LOGD(state_name(), "State transition-> %s",state_name(get_next_state()));
  }
  void state_machine_class::handle_state_transition(){
    if(get_next_state() != state() && get_next_state() != NONE){
      prev_state = current_state;
      current_state = get_next_state();
      state_start_time = get_run_time();
      entry_done = false;
      id(controller_state).publish_state(state_name());
      ESP_LOGD(state_name(),"State transition complete-> %s",state_name());
    }
  }
  const char * state_machine_class::state_friendly_name(states stt){
    if(stt == NONE) stt = current_state;
    static const std::string state_string_friendly_list[13] = {"None","Initialiseren","Uit","Start","Opstarten","Aan (stabiliseren)","Aan (verwarmen)","Aan (overshoot)","Aan (stall)","Pauze (Uit)","Aan (Warm Water)","Ontdooien","Nadraaien"}; 
    return state_string_friendly_list[stt].c_str();
  }
  const char * state_machine_class::state_name(states stt){
    if(stt == NONE) stt = current_state;
    static const std::string state_string_list[13] = {"NONE","INIT","IDLE","START","STARTING","STABILIZE","RUN","OVERSHOOT","STALL","WAIT","SWW","DEFROST","AFTERRUN"};
    return state_string_list[stt].c_str();
  }
  uint_fast32_t state_machine_class::get_run_time(){
    return run_time_value;
  }
  void state_machine_class::increment_run_time(uint_fast32_t increment){
    run_time_value += increment;
  }
  void state_machine_class::set_run_start_time(){
    run_start_time = get_run_time();
  }
  uint_fast32_t state_machine_class::get_run_start_time(){
    return run_start_time;
  }
  uint_fast32_t state_machine_class::get_state_start_time(){
    return state_start_time;
  }
  uint_fast32_t state_machine_class::seconds_since_state_start(){
    return get_run_time() - get_state_start_time();
  }
  //receive all values, booleans (states) or floats (values)
  void state_machine_class::receive_inputs(){
    input[THERMOSTAT_SENSOR]->receive_state(id(thermostat_signal).state); //state of thermostat input
    input[THERMOSTAT]->receive_state(thermostat_state());
    input[COMPRESSOR]->receive_state(id(compressor_running).state); //is the compressor running
    input[SWW_RUN]->receive_state(id(sww_heating).state); //is the domestic hot water run active
    input[DEFROST_RUN]->receive_state(id(defrosting).state); //is defrost active
    input[OAT]->receive_value(round(id(buiten_temp).state)); //outside air temperature
    if(input[OAT]->has_flag() || update_stooklijn_bool) input[STOOKLIJN_TARGET]->receive_value(calculate_stooklijn()); //stooklijn target
    //Set to value that anti-pendel script will track (outlet/inlet) (recommend inlet)
    input[TRACKING_VALUE]->receive_value(floor(id(water_temp_aanvoer).state));
    input[BOOST]->receive_state(id(boost_switch).state);
    input[BACKUP_HEAT]->receive_state(id(relay_backup_heat).state); //is backup heat on/off
    input[EXTERNAL_PUMP]->receive_state(id(relay_pump).state); //is external pump on/off
    input[RELAY_HEAT]->receive_state(id(relay_heat).state); //is realy_heat (heatpump external thermostat contact) on/off
    input[WP_PUMP]->receive_state(id(pump_running).state); //is internal pump running
    input[SILENT_MODE]->receive_state(id(silent_mode_state).state); //is silent mode on
    if(input[TEMP_NEW_TARGET]->value == 0.0) input[TEMP_NEW_TARGET]->value = input[STOOKLIJN_TARGET]->value; // set temp new target
    delta = input[TRACKING_VALUE]->value-input[STOOKLIJN_TARGET]->value; 
    pendel_delta = input[TRACKING_VALUE]->value-input[TEMP_NEW_TARGET]->value;
  }
  void state_machine_class::process_inputs(){
    if(input[BOOST]->state){
      if(input[BOOST]->seconds_since_change() > (id(boost_time).state*60)) boost(false);
    }
    if(input[BOOST]->has_flag()){
      toggle_boost();
    }
    toggle_silent_mode();
    if(input[WP_PUMP]->state && state() != SWW && state() != DEFROST){
      //calculate derivative and publish new value
      calculate_derivative(input[TRACKING_VALUE]->value);
    } else if(!input[WP_PUMP]->state && derivative.size() > 0){
      //if pump not running and derivative has values clear it
      derivative.clear();
      id(derivative_value).publish_state(0);
    }
  }
  //set input.value.prev_value = input_value.value to remove the implicit 'value changed' flag
  void state_machine_class::unflag_input_values(){
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
    input[WP_PUMP]->unflag();
    input[SILENT_MODE]->unflag();
    input[EMERGENCY]->unflag();
  }
  //***************************************************************
  //*******************Stooklijn***********************************
  //***************************************************************
  //calculate stooklijn function
  float state_machine_class::calculate_stooklijn(){
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
      update_stooklijn_bool = false;
    }
    float new_stooklijn_target;
    //OAT expects start temp to be OAT 20 with Watertemp 20. Steepness is defined bij Z, calculated by the max wTemp at minOat
    //Formula is wTemp = ((Z x (stooklijn_max_oat - OAT)) + stooklijn_min_wtemp) + C 
    //Formula to calculate Z = 0-((stooklijn_max_wtemp-stooklijn_min_wtemp)) / (stooklijn_min_oat - stooklijn_max_oat))
    //C is the curvature of the stooklijn defined by C = (stooklijn_curve*0.001)*(oat-max_oat)^2
    //This will add a positive offset with decreasing offset. You can set this to zero if you don't need it and want a linear stooklijn
    //I need it in my installation as the stooklijn is spot on at relative high temperatures, but too low at lower temps
    const float Z =  0 - (float)( (id(stooklijn_max_wtemp).state-id(stooklijn_min_wtemp).state)/(id(stooklijn_min_oat).state - id(stooklijn_max_oat).state));
    //If oat above or below maximum/minimum oat, clamp to stooklijn_max/min value
    float oat_value = input[OAT]->value;
    if(oat_value > id(stooklijn_max_oat).state) oat_value = id(stooklijn_max_oat).state;
    else if(oat_value < id(stooklijn_min_oat).state) oat_value = id(stooklijn_min_oat).state;
    float C = (id(stooklijn_curve).state*0.001)*pow((oat_value-id(stooklijn_max_oat).state),2);
    new_stooklijn_target = (int)round( (Z * (id(stooklijn_max_oat).state-oat_value)) + id(stooklijn_min_wtemp).state + C);
    //Add stooklijn offset
    new_stooklijn_target = new_stooklijn_target + id(wp_stooklijn_offset).state;
    //Add boost offset
    new_stooklijn_target = new_stooklijn_target + current_boost_offset;
    //Clamp target to minimum temp/max water+3
    clamp(new_stooklijn_target,id(stooklijn_min_wtemp).state,id(stooklijn_max_wtemp).state+3);
    ESP_LOGD("calculate_stooklijn", "Stooklijn calculated with oat: %f, Z: %f, C: %f offset: %f, result: %f",input[OAT]->value, Z, C, id(wp_stooklijn_offset).state, new_stooklijn_target);
    //Publish new stooklijn value to watertemp value sensor
    id(watertemp_target).publish_state(new_stooklijn_target);
    return new_stooklijn_target;
  }
  //***************************************************************
  //*******************Thermostat**********************************
  //***************************************************************
  bool state_machine_class::thermostat_state(){
    //if sensor and thermostat are the same, just return
    if(input[THERMOSTAT_SENSOR]->state == input[THERMOSTAT]->state) return input[THERMOSTAT]->state;
    //instant on
    if(state() == INIT && input[THERMOSTAT_SENSOR]->state) return true;
    //thermostat change
    if(input[THERMOSTAT_SENSOR]->state){
      //state change is a switch to on
      //check if on delay has passed
      if(input[THERMOSTAT_SENSOR]->seconds_since_change() > (id(thermostat_on_delay).state*60)) return true;
    } else {
      //state change is a switch to off
      //check for instant off
      if(!input[COMPRESSOR]->state || state() == SWW || state() == DEFROST) return false;
      //check if off delay time has passed
      if(input[THERMOSTAT_SENSOR]->seconds_since_change() > (id(thermostat_off_delay).state*60)) {
        //then check if minimum run time has passed
        if((get_run_time() - run_start_time) > (id(minimum_run_time).state * 60))return false;
      }
    }
    //return previous state
    return input[THERMOSTAT]->state;
  }
  //***************************************************************
  //*******************Derivative**********************************
  //***************************************************************
  void state_machine_class::calculate_derivative(float tracking_value){
    float d_number = tracking_value;
    derivative.push_back(d_number);
    //limit size to 31 elements(15 minutes)
    if(derivative.size() > 31) derivative.erase(derivative.begin());
    //calculate current derivative for 5 and 10 minutes
    //derivative is measured in degrees/minute
    derivative_D_5 = 0;
    derivative_D_10 = 0;
    //wait until derivative > 14, this makes sure the first 2 minutes are skipped
    //first minute or so is unreliabel if pump has been off for a while (water cools in the unit)
    if(derivative.size() > 14){   
      derivative_D_5 = (derivative.back()-derivative.at(derivative.size()-11))/10;
    }
    if(derivative.size() > 24){   
      derivative_D_10 = (derivative.back()-derivative.at(derivative.size()-21))/20;
    }
    //make sure there is always a prediction even with derivative = 0
    pred_20_delta_5 = (tracking_value + (derivative_D_5*20)) - input[STOOKLIJN_TARGET]->value;
    pred_20_delta_10 = (tracking_value + (derivative_D_10*20)) - input[STOOKLIJN_TARGET]->value;
    pred_5_delta_5 = (tracking_value + (derivative_D_5*5))- input[STOOKLIJN_TARGET]->value;
    //publish new value
    id(derivative_value).publish_state(derivative_D_10*60);
  }
  //***************************************************************
  //*******************Heat****************************************
  //***************************************************************
  void state_machine_class::heat(bool mode){
    if(mode){
      if(!id(relay_heat).state) {
        id(relay_heat).turn_on();
        input[RELAY_HEAT]->receive_state(true);
      }
      //if relay heat is turned on, relay_pump must also be turned on
      if(!input[EXTERNAL_PUMP]->state){
        external_pump(true);
        ESP_LOGD(state_name(),"Invalid configuration relay_heat on before relay_pump.");
        id(controller_info).publish_state("Invalid config: heat on before pump.");
      }
    } else {
      if(id(relay_heat).state) {
        id(relay_heat).turn_off();
        input[RELAY_HEAT]->receive_state(false);
      }
      //external pump can remain on, backup heater must be off
      if(input[BACKUP_HEAT]->state){
        backup_heat(false);
        ESP_LOGD(state_name(),"Invalid configuration relay_heat off before relay_backup_heat off.");
        id(controller_info).publish_state("Invalid config: heat off before backup_heat");
      }
    }
  }
  //***************************************************************
  //*******************Pump****************************************
  //***************************************************************
  void state_machine_class::external_pump(bool mode){
    if(mode){
      if(!id(relay_pump).state) {
        id(relay_pump).turn_on();
        input[EXTERNAL_PUMP]->receive_state(true);
      }
    } else {
      if(input[RELAY_HEAT]->state){
        heat(false);
        ESP_LOGD(state_name(),"Invalid configuration relay_pump off before relay_heat");
        id(controller_info).publish_state("Invalid config: pump off before heat");
      }
      //backup heater must be off
      if(input[BACKUP_HEAT]->state){
        backup_heat(false);
        ESP_LOGD(state_name(),"Invalid configuration relay_pump off before relay_backup_heat");
        id(controller_info).publish_state("Invalid config: pump off before backup_heat");
      }
      if(id(relay_pump).state){
        id(relay_pump).turn_off();
        input[EXTERNAL_PUMP]->receive_state(false);
      }
    }
  }
  //***************************************************************
  //*******************Backup Heat*********************************
  //***************************************************************
  void state_machine_class::backup_heat(bool mode,bool temp_limit_trigger){
    if(mode){
      //relay heat must be on, otherwise it is an invalid request
      if(!input[RELAY_HEAT]->state){
        //do not turn on
        ESP_LOGD(state_name(),"Invalid configuration relay_backup_heat on before relay_heat.");
        id(controller_info).publish_state("ERROR: backup_heat on before heat.");
      } else {
        if(!id(relay_backup_heat).state){
          id(relay_backup_heat).turn_on();
          input[BACKUP_HEAT]->receive_state(true);
          if(temp_limit_trigger) {
            backup_heat_temp_limit_trigger = true;
            id(controller_info).publish_state("Backup heat on due to low temp");
          } else if(input[SWW_RUN]->state) {
            id(controller_info).publish_state("Backup heat on due to SWW run");
          } else if(input[DEFROST_RUN]->state) {
            id(controller_info).publish_state("Backup heat on due to Defrost");
          } else if(state() == STALL){
            id(controller_info).publish_state("Backup heat on due to STALL");
          } else {
            id(controller_info).publish_state("Backup heat on");
          }
        }
        //if relay_backup_heat is turned on, relay_pump must also be turned on
        if(!input[EXTERNAL_PUMP]->state){
          external_pump(true);
          ESP_LOGD(state_name(),"Invalid configuration relay_backup_heat on before relay_pump.");
          id(controller_info).publish_state("Invalid config: backup_heat on before pump.");
        }
        backup_heat_temp_limit_trigger = false;
      }
    } else {
      if(id(relay_backup_heat).state){
        id(relay_backup_heat).turn_off();
        input[BACKUP_HEAT]->receive_state(false);
        backup_heat_temp_limit_trigger = false;
      } 
      //all else can remain on
    }
  }
  //***************************************************************
  //*******************Boost***************************************
  //***************************************************************
  void state_machine_class::boost(bool mode){
    if(mode){
      if(!input[BOOST]->state) id(boost_switch).turn_on();
    } else {
      if(input[BOOST]->state) id(boost_switch).turn_off();
    }
  }
  void state_machine_class::toggle_boost(){
    if(input[BOOST]->state){
      current_boost_offset = boost_offset;
      input[STOOKLIJN_TARGET]->receive_value(calculate_stooklijn());
      id(controller_info).publish_state("Boost mode active");
    } else {
      current_boost_offset = 0;
      input[STOOKLIJN_TARGET]->receive_value(calculate_stooklijn());
      id(controller_info).publish_state("Boost mode deactivated");
    }
  }
  //***************************************************************
  //*******************Silent mode logic***************************
  //***************************************************************
  void state_machine_class::silent_mode(bool mode){
    if(mode){
      if(!input[SILENT_MODE]->state){
        id(silent_mode_switch).turn_on();
        id(silent_mode_state).publish_state(true);
        input[SILENT_MODE]->receive_state(true);
      }
    } else {
      if(input[SILENT_MODE]->state){
        id(silent_mode_switch).turn_off();
        id(silent_mode_state).publish_state(false);
        input[SILENT_MODE]->receive_state(false);
      }
    }
  }
  void state_machine_class::toggle_silent_mode(){
    //if input[OAT]->value >= silent always on: silent on
    //if input[OAT]->value <= silent always off: silent off
    //if in between: if boost or stall silent off otherwise silent on

    if(input[OAT]->value >= id(oat_silent_always_on).state) {
      if(!input[SILENT_MODE]->state){
        ESP_LOGD(state_name(),"oat > oat_silent_always_on and silent mode off, switching silent mode on");
        id(controller_info).publish_state("Switching Silent mode on oat > on");
        silent_mode(true);
      }
    } else if(input[OAT]->value <= id(oat_silent_always_off).state){
      if(input[SILENT_MODE]->state){
        ESP_LOGD(state_name(),"Oat < oat_silent_always_off Switching silent mode off");
        id(controller_info).publish_state("Switching silent mode off oat < oat_silent_always_off"); 
        silent_mode(false);
      }
    } else {
      if(input[BOOST]->state || state() == STALL) {
        if(input[SILENT_MODE]->state){
          ESP_LOGD(state_name(),"OAT between silent mode brackets. Boost or stall silent mode off");
          id(controller_info).publish_state("STALL/Boost switching silent mode off");
          silent_mode(false);
        }
      } else if(!input[SILENT_MODE]->state){
        ESP_LOGD(state_name(),"OAT between silent mode brackets. No boost/stall switching silent on");
        id(controller_info).publish_state("Switching silent mode on oat in between");
        silent_mode(true);
      }
    }
  }
  int state_machine_class::get_target_offset(){
    if(input[OAT]->value >= 10) return -3;
    if(input[OAT]->value >= id(oat_silent_always_on).state) return -2;
    return -1;
  }
  void state_machine_class::set_new_target(float new_target){
    //TODO check for multiple target changes during run
    if(new_target != input[TEMP_NEW_TARGET]->value) input[TEMP_NEW_TARGET]->receive_value(new_target);
  }
  void state_machine_class::start_events(){
    events.clear();
  }
  void state_machine_class::add_event(input_types ev){
    events.push_back(ev);
  }
  bool state_machine_class::check_change_events(){
    std::vector<input_types>::iterator it;
    bool state_change = false;
    for(it = events.begin(); it != events.end(); it++){
      if(*it == DEFROST_RUN){
        if(input[DEFROST_RUN]->state) {
          state_transition(DEFROST);
          ESP_LOGD(state_name(), "DEFROST run detected next state: DEFROST");
          state_change = true;
        }
      } else if(*it == SWW_RUN){  
        if(input[SWW_RUN]->state && !input[DEFROST_RUN]->state) {
          state_transition(SWW);
          ESP_LOGD(state_name(), "SWW run detected next state: SWW");
          state_change = true;
        }
      } else if(*it == THERMOSTAT){
        if(!input[THERMOSTAT]->state) {
          if(!input[SWW_RUN]->state && !input[DEFROST_RUN]->state) {
            state_transition(AFTERRUN);
            ESP_LOGD(state_name(), "THERMOSTAT OFF next state: AFTERRUN");
            state_change = true;
          } else {
            backup_heat(false);
            heat(false);
            state_change = false;
          }
        }
      } else if(*it == RELAY_HEAT){
        if(!input[RELAY_HEAT]->state){
          //relay_heat switched off. Check if thermostat still on (or on again)
          if(input[THERMOSTAT]->state){
            external_pump(true);
            heat(true);
            ESP_LOGD(state_name(), "RELAY_HEAT OFF, but thermostat_sensor on switched relay_heat back on");
            id(controller_info).publish_state("Heat switched off; thermostat on. Heat back on");
          } else if(!input[SWW_RUN]->state && !input[DEFROST_RUN]->state) {
            state_transition(AFTERRUN);
            ESP_LOGD(state_name(), "RELAY_HEAT OFF next state: AFTERRUN");
            id(controller_info).publish_state("Heat switched off. Aborting");
            state_change = true;
          }
        }
      } else if(*it == COMPRESSOR){
        if(!input[COMPRESSOR]->state){
          //COMPRESSOR switched off. Failed run
          state_transition(WAIT);
          ESP_LOGD(state_name(), "Failed run detected next state: WAIT");
          state_change = true;
        }
      } else if(*it == EMERGENCY){
        if(pendel_delta >= hysteresis){
          state_transition(OVERSHOOT);
          state_change = true;
        }
      } else if(*it == BACKUP_HEAT){
        if(input[BACKUP_HEAT]->state){
          if(!input[RELAY_HEAT]->state || !input[THERMOSTAT]->state) {
            heat(false);
            backup_heat(false);
            ESP_LOGD(state_name(),"Backup heat off no heat request (relay_heat off)");
            id(controller_info).publish_state("Backup heat off due to no heat request");
          } else if(input[OAT]->value > id(backup_heater_active_temp).state){
            backup_heat(false);
            ESP_LOGD(state_name(),"Backup heat off input[OAT]->value > backup_heater_active_temp");
            id(controller_info).publish_state("Backup heat off due to high oat");
          } else if(backup_heat_temp_limit_trigger && input[OAT]->value > id(backup_heater_always_on_temp).state){
            //if triggered due to low temp and situation improved (with some hysteresis)
            backup_heat(false);
            ESP_LOGD(state_name(),"Backup heat off due to temperature improved");
            id(controller_info).publish_state("Backup heat off due to temperature improvement");
          }
        }
      }
    }
    events.clear();
    return state_change;
  }
  bool state_machine_class::compressor_modulation(){
    if(input[SILENT_MODE]->state && id(compressor_rpm).state <= 50) return true;
    else if(!input[SILENT_MODE]->state && id(compressor_rpm).state <= 70) return true;
    else return false;
  }
  bool state_machine_class::check_low_temp_trigger(){
    return (input[OAT]->value <= id(backup_heater_always_on_temp).state);
  }
  //update target temp through modbus
  void state_machine_class::set_target_temp(float target){
    auto water_temp_call = id(water_temp_target_output).make_call();
    water_temp_call.set_value(round(target));
    water_temp_call.perform();
    ESP_LOGD("set_target_temp", "Modbus target set to: %f", round(target));
    id(doel_temp).publish_state(target*10);
  }
  
  //main state machine object
  static state_machine_class fsm;