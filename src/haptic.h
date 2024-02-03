#pragma once

#ifndef HAPTIC_H
#define HAPIC_H

#include <Arduino.h>
#include <SimpleFOC.h>
#include <SimpleFOCDrivers.h>
#include "encoders/mt6701/MagneticSensorMT6701SSI.h"
#include <motor.h>
#include "foc_thread.h"


typedef struct
{
    // BEGIN NANO CONFIG

    // Profile Settings

    char profile_id = '0001'; // Profile unique ID generated by frontend
    char profile_desc = 'Profile Descritpion Within 50 Charcters'; // Maximum of 50 characters
    char profile_name = 'Default Profile Name'; // Maximum of 20 characters 
    /* 
        Maybe worth to consider to have profile_name and profile_display_name.
        Its possible that user might want to keep different profile name in profile list and display different name in nano GUI.
    */

    // Feedback Settings

    uint8_t profile_type = 0; 

    /* 
        0 - Fine Detents (Torque Mode)
        1 - Coarse Detents (Torque Mode)
        2 - Viscous Friction (Velocity Mode) 
        3 - Return To Centre (Torque Mode)

        What is function of these?

        Profile Type [0,1] are not fulfilling any function in firmware, 
        these are just to be read by front-end and then UI configuration 
        options are adapted depending on type. Haptic Firmware handles the rest.

        Profile Type 0 - UI locks attractor angle range between 2-6 degrees
        Profile Type 1 - UI locks attractor angle range between 7-180 degrees

        ###

        Profile Type [3] tells firmware to send current position as negative
        or positive number which is rounded up degree in range of [-180 ... 180].
        In this mode the default position is always 0, and as knob is being rotated CW or CCW the angular difference
        CCW [-180...0]; CW [0...180]

        Profile Type 3 - UI limits position number to 1 and does not render users ability to modify attractor angle

        ###

        Profile type [2] tells firmware switch to MotionController::Velocity
        Profile Type 2 - UI locks renames attraction angle to position distance and locks range between 1-12,
        On Firmware side this value is still defined as attract_distance
    */

    uint16_t position_num = 120;

    /*
        Total number of defined positions 
        UI -> Total Positions
        For 'Fine Detents', 'Coarse Detents' and 'Viscous Friction' these values must be between [2...65535]
        For Return To Centre this is always 1 and if this mode is selected user cannot ented number of detents in Front-End UI
    */

    uint8_t attract_distance = 4; 

    /*
        Distance between virtual points represented in Degrees, these shouldn't be float values i think. 
        User is likely to never notice difference in decimal adjustment.
        Firmware then converts that vallue from dergees to radians.
    */

    uint8_t feedback_strength = 6;

    /*
        Feedback Strength modifies haptic_pid->limit, estimated current,
        Values from [1...6]
        Input values then are divided by factor of 10. Output should be decimal eg, 0.1 - 0.6 (A estimated)
        Note: In my experience there is really no need to make this more granular, 
        with current algorithm anything over 600mA renders feedback choppy 
        (Need to investigate maybe this is related to incorrect phase resistance should phase resistance be measured value or measured value / 2 eg)

        If it doesnt matter whether this value is int or float, maybe better to be able to send float instead
    */

    uint8_t bounce_strength = 3;

    /*
        Bounce Strength modifies haptic_pid->limit, estimated current,
        Values from [1...6]
        Input values then are divided by factor of 10. Output should be decimal eg, 0.1 - 0.6 (A estimated)

        If it doesnt matter whether this value is int or float, maybe better to be able to send float instead

    */

    uint8_t haptic_click_strength = 6;

    /* 
        Click Strength modifies hapic_pid->limit, estimated current.
        Values from [1...6]
        This is being used by HapticInterface::haptic_click function
    */

    uint16_t output_ramp = 10000;

    /*
        Feedback Strength modifies haptic_pid->output_ramp
        Values from [2...10] with steps every 2000 (no float values)
        Input values are then multiplied by factor of 10000.

    */

    uint8_t audio_feedback_lvl = 3;

    /* 
        Audio Feedback Level modifies I2S library Volume [0...255]
        Values from [1...5]
        Input values are then mapped map(audio_feedback, 1,5 0,255)
    */

    uint8_t audio_file = 2;

    /*
        Audio File (UI: Audio Magnitude) set pre-uploaded audio file which can be stored as PROGMEM or in SPIFS
        Audio files are very small, in most of cases way under 500ms, stored as mono 16bit 24Khz samples, 
        Audio Files: 'faint' [0], 'soft' [1], 'default' [2], 'medium' [3], 'hard' [4],
        Note: It is not intended or for user to be able to upload or tune these pre-defined sound files. 
        Some documentation on sound design will be provided in User Documentation, 
        this however could be modified only by advanced users who want to play with source.

        TODO: In future to consider generative sound approach with virtual oscillator (sine, sqare, saw)?

    */

    // Mapping Configuration

    bool is_midi = false;

    /* 
        Toggles between USB (slave) or MIDI (standalone) operation, 
        In UI -> Profile Settings -> Connection Type
        If is_midi = 1, midi_thread is enabled and listens for 
        buttons or knob state update that contains CC message, else midi_thread is disabled

    */

    /* 

        Context:
        Mapping Buttons can play few roles:

        Case 1: Mapped Button can trigger another profile (eg. pres button -> go to next profile withing same group [tag] or go to specific profile [uid])
        Case 2: Mapped Button can toggle current/active CC # - knob sends values 0-127 the output message
        Case 3: Mapped Button can send specific CC # with specific value
        Case 4: Mapped Button contains mapping value that can be parsed by RobotJS (eg. Key 'B' or 'Arrow Up' or 'Ctrl')
        TODO: how mappings for buttons and knob should be stored as?

    */


    char button_A_map = '1016001255'; //  MODE: Midi(1), TRIGGER: Press(0), MIDI CH (16), CC#(01), CC(255)
    char button_B_map = '214567'; // MODE: Internal(2), TRIGGER: Hold(1), UID: (4567)
    char button_C_map = '0'; // No mapping -> Disables button and RGB assigned for that button
    char button_D_map = '0'; //etc.

    /*
    Each button mapping should contain list of parameters:
        Parameter 0 - Type of Mapping (?)
        Parameter 1 - Mapping Trigger (Eg. Single Press or Hold)
        Parameter 2 - Mapping Type (Eg. Internal, Virtual HID, MIDI)
        Parameter 3 - Mapping Value (Eg. CC# + optional CC Value)


        Mapping Type Definitions:

            Type 1 Internal:
                Button is can be set to toggle between profiles.
                Toggle can:
                    - Temporarily switch to another profile while button is being held and revert to previous if released
                    - Button can switch to another profile when button is pressed
                    - Button can switch to next/previous profile (in the group [tag]) on press.
                    - Button can enable browse mode and knob can be used to toggle between profiles for quick access

            Type 2 Virtual HID:
                - Knob is by default assigned to some computer (Served by RobotJS) readable value 
                - Button can be assigned to any computer (Served by RobotJS) readable keystroke value
                - Button on press/hold can assign new value to Knob

            Type 3 MIDI:
                - Knob or Button can have set different MIDI Channel
                - Knob can hav assigned CC# and CC Value is updated on rotation CW or CCW
                - Button Can have assigned CC# and fixed CC Value
                - Button can modify CC# value of Knob (press/hold)

        Mapping Values:

            If MIDI is assigned to key:
                [type][trigger][channel][cc_num][cc_val]
                1016001255 - Midi(1), Trigger(0) - Press, Midi Channel 16, CC#01, CC 255

            If Virtual HID is assigned to key
                [type][string] (?)
                0STRING - USB(0), Some string (?)
                Some computer readible value is being passed that is further interpreted by RobotJS

            If Internal is assigned to key
                [type][trigger][uid]
                214567 - Internal(2), Trigger(1) - Hold, UID4567


        Trigger:
            Press -> Permanent Change
            Hold -> Temporary Change, on release revert to previous

        Above is highly theoritical and not ultimate solution, this should be discussed on best approach.
        Are there good common practives or case study that we could model mapping model and way the values are being stored 
        and then interpreted by firmware and softwae reliably?

    */



    // Light Settings

    bool is_led = true; 

    // If True FastLED is enabled

    uint8_t led_mode = 0;

    /*
        Led Mode toggles between 4 pre-defined led feedback variants
        0 - Solid Color (Unreactive)
        1 - Pulse (Breathing Animation which uses Delay in for loop)
        2 - Half Follow Pointer (Active half is filled with Primary color + pointer [current_pos])
        3 - Full Follow Pointer (Active half is filled with Primary Color + pointer [current_pos ] + Inactive Half is filled with Secondary Color)

        Note:
        Led Mode can be dynamic in operation eg. Upon holding key [A...D] led_mode can be modified to 1 (Pulse), 
        Upon releasing it would return to default value stored in profile config.
    */


    // LED Ring array: Pointer, Primary, Scondary always store only one HEX value
    char pointer_col = '0xC0FFEE';
    char prmary_col = '0xC0FFEE';
    char secondary_col = '0xC0FFEE';

    char button_A_col_idle = '0xC0FFEE'; 
    char button_B_col_idle = '0xC0FFEE'; 
    char button_C_col_idle = '0xC0FFEE'; 
    char button_D_col_idle = '0xC0FFEE'; 

    char button_A_col_press = '0xFFFFFF';
    char button_B_col_press = '0xFFFFFF';
    char button_C_col_press = '0xFFFFFF';
    char button_D_col_press = '0xFFFFFF';


    /* 
        Note: 
        Unsure how these properties can be stored more efficiently without having too many variables.
        Perhaps idle and press color of each button could be stored in one value as array?
        Should these values be 3byte each? Is it maybe better to store color values for buttons in Button Mapping?

    */

    // Gui Settings

    /*
        This entire section can be ignored for now. In future GUI settings can contain following parameters:
        uint8_t primaryGuiFont = 1 - Device would have pre-defined fonts, user is not able to update own font.
        uint8_t secondaryGuiFont = 0 - Device would have pre-defined fonts, user is not able to update own font.
        uint8_t styleGui = 1 - Device can have pre-defined GUI styles/templates, user can choose between 2-3 variants per profile. LCD_THREAD handles its interpretation
        bool devGuiOverlay = - Optional advanced option that could show current FPS or highlight/outilne currently refreshed area of the buffer (?)
    */

    // END NANO CONFIG

    // ###################################

    // BEGIN NANO STATE
    // THis is just temporary to keep the program working
    // TODO: Separate config from state. And define what are config and state

    // Default Haptic Program Configuration
    uint8_t start_pos = 1;
    uint16_t end_pos = position_num;
    uint16_t total_pos = end_pos - start_pos +1;

    uint16_t last_pos = 0; // Record last position for a recall
    float attract_angle = 0; // angle where PID will point to
    float last_attract_angle = 0;

    // Current Position Behavior
    uint16_t current_pos = 1; // Current Actual Position

    /* 
    Current position is always represented as integer
    
    */

    uint8_t detent_count = 0;

    float distance_pos = attract_distance * _PI / 180; // Define attractior distance position and convert it to radians

    float detent_strength_unit = 0.2; // PID (estimated) Current Limit

    float endstop_strength_unit = 0.3; // PID (estimated) Current Limit

    float click_strength = 0.4; //  PID (estimated) Current Limit
    
    // END NANO STATE
} hapticState;


typedef struct
{

    // Exponentially Weighted Moving Average (EWMA)
    // Used to correct PID, if position near alignment centre
    // but not there yet, adjust slowly to the centre
    // TODO: Implement EWMA and compare perfromance w/wo

    float dead_zone_percent = 0.2;
    float dead_zone_rad = _PI/180;

    float idle_velocity_ewma_alpha = 0.001;
    float idle_velocity_rad_per_sec = 0.05;
    float idle_check_velocity_ewma = 0;
    float idle_correction_max_angle_rad = 5 * _PI/180;
    float idle_correction_rate_alpha = 0.0005;

    float last_idle_start = 0;
    uint32_t idle_correction_delay_millis = 500;

} hapticParms;

class HapticInterface
{
public:   
    hapticState* haptic_config;
    hapticParms* haptic_params;
    BLDCMotor* motor;
    PIDController* haptic_pid;

    // All the various constructors.
    HapticInterface(BLDCMotor* _motor);
    HapticInterface(BLDCMotor* _motor, PIDController* _pid);
    HapticInterface(BLDCMotor* _motor, hapticState* _config);
    HapticInterface(BLDCMotor* _motor, PIDController* _pid, hapticState* _config);
    HapticInterface(BLDCMotor* _motor, PIDController* _pid, hapticState* _config, hapticParms* _parms);


    void init(void);
    void haptic_loop(void);
    void haptic_click(void);

protected:
private:
    void find_detent(void);
    void update_position(void);
    void state_update(void);
    float haptic_target(void);
    void correct_pid(void);
};

#endif