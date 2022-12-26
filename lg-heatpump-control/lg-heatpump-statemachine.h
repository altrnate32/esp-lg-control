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
        std::vector<float> cop_vector; //vector of floats to integrate cop
        float average_cop = 0; //keep track off average cop during last 15 minutes
        bool backup_heat_cop_limit_trigger = false; //if backup heat triggered due to low cop?
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
        float backup_heater_cop_limit = 1.5; //below which cop should the backup heater be switched on? Set to a large negative number (example -1000) to disable 
        float derivative_D_5 = 0; //derivative based on past 5 minutes
        float derivative_D_10 = 0; //derivative based on past 10 minutes
        float pred_20_delta_5 = 0; //predicted delta in 20 minutes based on last 5 minute derivative
        float pred_20_delta_10 = 0; //predicted delta in 20 minutes based on last 10 minute derivative
        float pred_5_delta_5 = 0; //predicted delta in 5 minutes based on last 5 minute derivative
        state_machine_class();
        ~ state_machine_class();
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
        void calculate_average_cop();
        void heat(bool mode);
        void external_pump(bool mode);
        void backup_heat(bool mode,bool cop_limit_trigger = false);
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
        bool check_low_cop_trigger();
        void set_target_temp(float target);
    };
