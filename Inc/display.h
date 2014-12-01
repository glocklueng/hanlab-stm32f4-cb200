#ifndef __CB_DISPLAY_H__
#define __CB_DISPLAY_H__

#pragma pack(1)
struct TProgram {
    // program
    bool rcf_mode;
    bool cooler_control_auto;
    signed char temp;
    unsigned char rotor_type;
    unsigned char accel;
    unsigned char decel;
    unsigned short rpm;
    unsigned short time;
};
#pragma pack()

#pragma pack(push, 1)
struct DEBUG_DATA
{
    unsigned char disp_option;

    union {
        struct {
            float vib;
            float phase;
            //float rms1;
            float rms2;
        } dsp;
        struct {
            unsigned char disp;
            short val[6];
        } debug;
    };

    unsigned char motor_temp;
	unsigned char motor_driver_temp;
};
#pragma pack(pop)

extern volatile int gRunStatus;
extern volatile bool gDisplayRCF;
extern volatile bool gDebugMode;
extern volatile struct DEBUG_DATA gDebugData;

void UI_CustomPopup(int ui_state, char* title, char* text, ...);
void UI_SendDispMsg(unsigned char cmd, unsigned char option, void* data, struct PROGRAM* cur_prg);

enum {
	DISP_BOOT = 0,
	DISP_RUN,
	DISP_SETTING,
	DISP_TEXT,
	DISP_NETWORK_SETTING,
	DISP_POPUP_TITLE_START,
	DISP_POPUP_TITLE_CONTINUE,
	DISP_POPUP_TEXT_START,
	DISP_POPUP_TEXT_CONTINUE,
};

enum {
	STATUS_ACCEL = 0x1,
	STATUS_STEADY = 0x2,
	STATUS_DECEL = 0x4,
	STATUS_COOLING = 0x8,
	STATUS_DOOROPEN = 0x10,
};

enum {
	POPUP_NONE = 0,
	POPUP_MSG_TTE,
	POPUP_ERR_HWERROR,
	POPUP_WARN_DOOR,
	POPUP_ERR_DOOR,
	POPUP_ERR_STILL_RUN,
	POPUP_MSG_TRY_RUN,
	POPUP_MSG_AUTOBAL_START,
	POPUP_MSG_AUTOBAL_STEP_START,
	POPUP_ERR_AUTOBAL_FAIL,
	POPUP_MSG_AUTOBAL_SUCCESS,
	POPUP_ERR_IMBALANCE,
	POPUP_MSG_TRY_CANCEL,
	POPUP_MSG_PROCESS_CANCEL,
	POPUP_MSG_PROCESS_END,
	POPUP_ERR_PROCESS_FAIL,
	POPUP_MSG_BALLBAL_START,
	POPUP_ERR_TEMP_MEASUREMENT,
	POPUP_ERR_VIB_MEASUREMENT,
	POPUP_ERR_OVER_BALANCING,
	POPUP_ERR_ROTOR_TYPE,
	POPUP_ERR_RPM_MEASUREMENT,
	POPUP_MSG_ROTOR_ADJUST,
	POPUP_ERR_ROTOR_COMM,
	POPUP_ERR_ROTOR_JAM,
	POPUP_ERR_MOTOR,
	POPUP_ERR_UNKNOWN,
	POPUP_MSG_WAIT_TEMP,
    POPUP_MSG_PROGRAM_INFO,
	POPUP_FLAT_CUSTOM = 0x3B,
	POPUP_MSG_CUSTOM = 0x3C,
	POPUP_WARN_CUSTOM = 0x3D,
	POPUP_ERR_CUSTOM = 0x3E,
	POPUP_QUESTION_CUSTOM = 0x3F,
	POPUP_FLAT_CUSTOM_WITH_TEXT = 0xBB,
	POPUP_MSG_CUSTOM_WITH_TEXT = 0xBC,
	POPUP_WARN_CUSTOM_WITH_TEXT = 0xBD,
	POPUP_ERR_CUSTOM_WITH_TEXT = 0xBE,
	POPUP_QUESTION_CUSTOM_WITH_TEXT = 0xBF,
};

enum
{
	DISP_OPTION_HAVE_COOLER = 0x1,
	DISP_OPTION_POPUP_USE_TEXT = 0x2,
	DISP_OPTION_PROGRESS_BAR_ON_AT_BOOT = 0x4,
	DISP_OPTION_EDIT_MODE_AT_SETTING = 0x4,
	DISP_OPTION_RCF_MODE_AT_RUN = 0x4,
	DISP_OPTION_VER_DISP_OFF_AT_BOOT = 0x8,
	DISP_OPTION_RPM_DISP_OFF_AT_RUN = 0x8,
	DISP_OPTION_DEBUG_DISP_ON_AT_RUN = 0x10,
	DISP_OPTION_DEBUG_VIB_MODE_AT_RUN = 0x20,
	DISP_OPTION_TEMP_DISP_OFF_AT_RUN = 0x40,
    POPUP_OPTION_USE_CUSTOM_TEXT = 0x80,

    DEBUG_DISP_WEIGHT_MOVE_PLAN = 0x1,
    DEBUG_DISP_WEIGHT_MOVE_COMMERR = 0x2,
    DEBUG_DISP_WEIGHT_MOVE_TIMEOUT = 0x3,
    DEBUG_DISP_WEIGHT_MOVE_RESULT = 0x4,
};

#pragma pack(push, 1)
struct TDisplayNetworkSetting {
	unsigned char  server_ip[4];
	unsigned short server_port;
	unsigned short telnet_wait_port;
	unsigned char  ip[4];
	unsigned char  mask[4];
	unsigned char  gateway[4];
	bool dhcp;
    unsigned char reset_flag;
};

struct TDisplayBoot {
	// version
	unsigned long main_ver; // date format (ex:20080922)

	// tte
	unsigned long tte1;
	unsigned long tte2;
	unsigned long tte3;
	unsigned long tte4;

	// option
	unsigned char disp_option;
	unsigned char progress;

	// popup
    unsigned char popup;
	long popup_option;
};

struct TDisplayRun {
	// data
    unsigned short cur_rpm;
    unsigned short end_rpm;
    unsigned short elapsed_time_secs;
    unsigned short total_time_secs;
    unsigned char status_and_rotor_type; // with rotor_type
    signed char cur_temp;
    signed char limit_temp;

	// option
	unsigned char disp_option;

	// popup
	unsigned char popup;
	unsigned char autobal_step_and_program_no; // with program_no

	// debug data
	union {
		struct {
			float vib;
			float phase;
			//float rms1;
			float rms2;
		} dsp;
		struct {
            unsigned char disp;
			short val[6];
		} debug;
	};

    unsigned char motor_temp;
	unsigned char motor_driver_temp;
    unsigned char accel_and_decel; // accel + decel
};

struct TDisplaySetting {
    // data
    unsigned short rpm;
    unsigned short time;
	bool rcf_mode;
    unsigned char rotor_type;
    unsigned char bucket_type;
    signed char temp;
    bool cooler_control_auto;
    unsigned char accel;
    unsigned char decel;
	unsigned char cur_program;

    // option
	unsigned char cursor_pos;
	unsigned char cursor_pos2;
	unsigned char disp_option;

	// popup
	unsigned char popup;
};
#pragma pack(pop)

#endif
