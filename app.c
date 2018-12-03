/*
*********************************************************************************************************
*                                               uC/OS-III
*                                         The Real-Time Kernel
*
*                             (c) Copyright 1998-2012, Micrium, Weston, FL
*                                          All Rights Reserved
*
*
*                                            PIC32 Sample code
*
* File : APP.C
*********************************************************************************************************
*/

#include <includes.h>
#include <bsp.h>
#define _SUPPRESS_PLIB_WARNING 1


//-----------------------------------------------------------------------
// ADC FUNCTIONS
//-----------------------------------------------------------------------
#define POT     0      // 10k potentiometer on AN2 input
// I/O bits set as analog in by setting corresponding bits to 0
#define AINPUTS 0xfffe // Analog inputs for POT pin A0 (AN2)
//Output Pins
#define GEAR_R LATBbits.LATB15
#define GEAR_N LATBbits.LATB14
#define GEAR_1 LATBbits.LATB8
#define GEAR_2 LATBbits.LATB9
#define GEAR_3 LATBbits.LATB10
#define GEAR_4 LATBbits.LATB11
#define GEAR_5 LATBbits.LATB12
#define GEAR_6 LATBbits.LATB13
#define BLUE LATBbits.LATB3
#define GREEN LATBbits.LATB4
#define RED LATBbits.LATB5

//Input Buttons
#define MODE_SWITCH BTN1
#define GEAR_DOWN BTN2
#define GEAR_UP BTN3
/*
#define MODE_SWITCH PORTBbits.RB1
#define GEAR_DOWN PORTBbits.RB2
#define GEAR_UP PORTBbits.RB3
*/
// initialize the ADC for single conversion, select input pins

void initADC(int amask) {
    AD1PCFG = amask; // select analog input pins
    AD1CON1 = 0x00E0; // auto convert after end of sampling
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F3F; // max sample time = 31Tad
    AD1CON1SET = 0x8000; // turn on the ADC
} //initADC

int readADC(int ch) {
    AD1CHSbits.CH0SA = ch; // select analog input channel
    AD1CON1bits.SAMP = 1; // start sampling
    while (!AD1CON1bits.DONE); // wait to complete conversion
    return ADC1BUF0; // read the conversion result
} // readADC

/*
*********************************************************************************************************
*                                                VARIABLES
*********************************************************************************************************
*/

static  OS_TCB    App_TaskStartTCB;
static  CPU_STK   App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  upShiftTCB;
static CPU_STK upShiftStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  downShiftTCB;
static CPU_STK downShiftStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  autoShiftTCB;
static CPU_STK autoShiftStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  manualShiftTCB;
static CPU_STK manualShiftStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  modeTCB;
static CPU_STK modeStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB  shiftLightTCB;
static CPU_STK shiftLightStk[APP_CFG_TASK_START_STK_SIZE];

//Mode is default to auto (0), manual (1);
int mode = 0;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_TaskCreate  (void);
static  void  App_ObjCreate   (void);

static  void  App_TaskStart   (void  *p_arg);

static  void  upShift(void *data);
static  void  downShift(void *data);
static  void  autoShift(void *data);
static  void  manualShift(void *data);
static  void  modeSwitch(void);
static  void  shiftLight(void *data);


//Global Vars and constants for shifting
const float MaxRPM = 1024;
int GearCurrent = 1;
const	int GearMax = 6;

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.
*
* Arguments   : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR   os_err;

    CPU_Init();
    /* Initialize the uC/CPU services                           */
    BSP_IntDisAll();

    OSInit(&os_err);
    /* Init uC/OS-III.                                          */


    OSTaskCreate((OS_TCB      *)&App_TaskStartTCB,
    /* Create the start task                                    */
                 (CPU_CHAR    *)"Start",
                 (OS_TASK_PTR  )App_TaskStart,
                 (void        *)0,
                 (OS_PRIO      )APP_CFG_TASK_START_PRIO,
                 (CPU_STK     *)&App_TaskStartStk[0],
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY   )0u,
                 (OS_TICK      )0u,
                 (void        *)0,
                 (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR      *)&os_err);

    OSStart(&os_err);                                                     /* Start multitasking (i.e. give control to uC/OS-III).     */

    (void)&os_err;

    return (0);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
* Arguments   : p_arg   is the argument passed to 'AppStartTask()' by 'OSTaskCreate()'.
*********************************************************************************************************
*/

static  void  App_TaskStart (void *p_arg)
{
    OS_ERR  err;


    (void)p_arg;

    BSP_InitIO();                                                       /* Initialize BSP functions                                 */

    Mem_Init();                                                 /* Initialize memory managment module                   */
    Math_Init();                                                /* Initialize mathematical module                       */

#if (OS_CFG_STAT_TASK_EN > 0u)
    OSStatTaskCPUUsageInit(&err);                               /* Determine CPU capacity                               */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

  initADC(AINPUTS); // init ADC
  AD1PCFG = 0xFFFF;
  // Define i/o modes
  TRISBbits.TRISB4 = 0;
  TRISBbits.TRISB5 = 0;
  TRISBbits.TRISB6 = 0;
  TRISBbits.TRISB7 = 0;
  TRISBbits.TRISB8 = 0;
  TRISBbits.TRISB9 = 0;
  TRISBbits.TRISB10 = 0;
  TRISBbits.TRISB11 = 0;
  TRISBbits.TRISB12 = 0;
  TRISBbits.TRISB13 = 0;
  TRISBbits.TRISB14 = 0;
  TRISBbits.TRISB15 = 0;
  /*
  TRISBbits.TRISB4 = 1;
  TRISBbits.TRISB3 = 1;
  TRISBbits.TRISB2 = 1;
  TRISBbits.TRISB1 = 1;
  */
  /*TRISBbits.BTN1 = 1;
  TRISBbits.BTN2 = 1;
  TRISBbits.BTN3 = 1;
  TRISBbits.BTN4 = 1;
   * */
  int gear = 0;
  int out = 1;


    App_TaskCreate();                                           /* Create Application tasks                             */

    App_ObjCreate();                                            /* Create Applicaiton kernel objects                    */

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */

      if( GEAR_UP )
      {
        gear++;
      }
      if( GEAR_DOWN )
      {
          gear--;
      }

      if ( gear > -1 || gear < 7) {
        GEAR_R = 0 ;
        GEAR_N = 0 ;
        GEAR_1 = 0 ;
        GEAR_2 = 0 ;
        GEAR_3 = 0 ;
        GEAR_4 = 0 ;
        GEAR_5 = 0 ;
        GEAR_6 = 0 ;
        switch(gear) {
          case 0:
            GEAR_N = 1;
          break;
          case 1:
            GEAR_1 = 1;
            break;
            case 2:
            GEAR_2 = 1;
            break;
            case 3:
            GEAR_3 = 1;
            break;
            case 4:
            GEAR_4 = 1;
            break;
            case 5:
            GEAR_5 = 1;
            break;
            case 6:
            GEAR_6 = 1;
            break;

        }
      }

    }
}

/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_TaskCreate (void)
{
    OS_ERR err;

    //Create Shift Up Task
    OSTaskCreate((OS_TCB *)&upShiftTCB,
            (CPU_CHAR *)"Up Shift",
            (OS_TASK_PTR)upShift,
            (void *)0,
            (OS_PRIO )3,
            (CPU_STK *)&upShiftStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create Shift Down Task
    OSTaskCreate((OS_TCB *)&downShiftTCB,
            (CPU_CHAR *)"Down Shift",
            (OS_TASK_PTR)downShift,
            (void *)0,
            (OS_PRIO )3,
            (CPU_STK *)&downShiftStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create Automatic Shift Task
    OSTaskCreate((OS_TCB *)&autoShiftTCB,
            (CPU_CHAR *)"Auto Shift",
            (OS_TASK_PTR)autoShift,
            (void *)0,
            (OS_PRIO )4,
            (CPU_STK *)&autoShiftStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create Manual Shift Task
    OSTaskCreate((OS_TCB *)&manualShiftTCB,
            (CPU_CHAR *)"Manual Shift",
            (OS_TASK_PTR)manualShift,
            (void *)0,
            (OS_PRIO )4,
            (CPU_STK *)&manualShiftStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create toggleMode task
    OSTaskCreate((OS_TCB *)&modeTCB,
            (CPU_CHAR *)"A/M mode Control",
            (OS_TASK_PTR)mode,
            (void *)0,
            (OS_PRIO )2,
            (CPU_STK *)&modeStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create task for shift light
    OSTaskCreate((OS_TCB *)&shiftLightTCB,
            (CPU_CHAR *)"Shift Light",
            (OS_TASK_PTR)shiftLight,
            (void *)0,
            (OS_PRIO )5,
            (CPU_STK *)&shiftLightStk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)512,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);
}


/*
*********************************************************************************************************
*                                          App_ObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_ObjCreate (void)
{
}

/*Task has highest priority, triggers the switch between aut and manual mode
 * mode = 0 for auto
 * mode = 1 for manual
*/
static void  modeSwitch(void)
{
    OS_ERR err;
    CPU_TS ts;



    while(1)
    {
        //Wait forever until interrupt triggers mode change
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);

        //Once ISR triggers function toggle mode
        if( mode == 0 )
        {
            mode = 1;
            //post to manual mode semaphore
            OSTaskSemPost(&manualShiftTCB,OS_OPT_POST_NONE,&err);
            //After mode toggled return to pending state
            break;
        }
        if( mode == 1 )
        {
            mode = 0;
            //post to auto mode semaphore
            OSTaskSemPost(&autoShiftTCB,OS_OPT_POST_NONE,&err);
            //After mode toggled return to pending state
            break;
        }
    }
}

//Transmission Functions
static void  autoShift(void *data)
{
    OS_ERR err;
    CPU_TS ts;

    float RPM = 0;

    while(1)
    {
        //Get RPM data
        RPM = readADC(POT);

        //Check if push button triggers manual mode
        if(MODE_SWITCH == 1 )
        {
            OSTaskSemPost(&modeTCB,OS_OPT_POST_NONE,&err);
        }

        //If manual mode activated pend and allow scheduler to go to manual mode
        if(mode == 1)
        {
            OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
        }
        //If RPM is within 5% of maximum shift up
        if(RPM >= (0.95*MaxRPM))
        {
            OSTaskSemPost(&upShiftTCB,OS_OPT_POST_NONE,&err);
            break;
        }
        //If RPM is falling shift to lower gear
        if(RPM <= 0.15*MaxRPM)
        {
            OSTaskSemPost(&downShiftTCB,OS_OPT_POST_NONE,&err);
            break;
        }
        
        //Now send the data to the shiftLight task and pend
        OSTaskSemPost(&shiftLightTCB,OS_OPT_POST_NONE,&err);
        OSTaskSemPend(10000, OS_OPT_PEND_BLOCKING, &ts, &err);

    }
}

//Task to shift using the paddle shifters
static void  manualShift(void *data)
{
    OS_ERR err;
    CPU_TS ts;
    float RPM = 0;

    //Pend waiting for autoShift to go to sleep
    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
    while(1)
    {
        //Get RPM data
        RPM = readADC(POT);

        //Check if push button triggers automatic mode
        if(MODE_SWITCH == 1 )
        {
            OSTaskSemPost(&modeTCB,OS_OPT_POST_NONE,&err);
        }


       //Check if return to auto shift mode required
       if(mode == 0)
       {
           OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
       }

       //Check if Neutral shift activated
       if( GEAR_UP && GEAR_DOWN )
       {
           GearCurrent = 0;
           break;
       }

       //UP Shift triggered
       else if( GEAR_UP )
       {
           OSTaskSemPost(&upShiftTCB,OS_OPT_POST_NONE,&err);
           break;
       }
       //Down Shift triggered
       else if( GEAR_DOWN )
       {
           OSTaskSemPost(&downShiftTCB,OS_OPT_POST_NONE,&err);
           break;
       }
       
       //Now send the data to the shiftLight task and pend
        OSTaskSemPost(&shiftLightTCB,OS_OPT_POST_NONE,&err);
        OSTaskSemPend(10000, OS_OPT_PEND_BLOCKING, &ts, &err);

    }
}

//Function to shift up one gear
static void  upShift(void *data)
{
    OS_ERR err;
    CPU_TS ts;
    
    //Wait forever until triggered for the first time
    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
    
    //Up Shift a gear once called
    while(1)
    {
        //Shift up a gear if not max gear
        if( GearCurrent != 6 && GearCurrent != 7)
        {
            GearCurrent = GearCurrent + 1;
        }
        
        //If Currently in reverse go to neutral
        else if( GearCurrent == 7)
        {
            GearCurrent = 0;
        }
        
        //Pend until triggered again and start loop over
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
        
    }
    
}

//Function to shift down one gear
static void  downShift(void *data)
{
    OS_ERR err;
    CPU_TS ts;
    
    //Wait forever until triggered for the first time
    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
    
    //Down Shift a gear once called
    while(1)
    {
        if( GearCurrent != 0 && GearCurrent != 7)
        {
            GearCurrent = GearCurrent - 1;
        }
        
        //If Currently in neutral go to reverse
        else if( GearCurrent == 0)
        {
            GearCurrent = 7;
        }
        
        //Pend until triggered again and start loop over
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
        
    }

}

static void  shiftLight(void *data)
{
    OS_ERR err;
    CPU_TS ts;
    int RPM = 1024;


    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
    while(1)
    {
        OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);

        if( RPM < (0.81*MaxRPM ))
        {
            //Light Green LED pin others 0
            GREEN = 1;
            BLUE = 0;
            RED = 0;
            break;
        }
        //Blue LED shift soon
        if( RPM >= (0.81*MaxRPM) && RPM < (0.96*MaxRPM))
        {
            BLUE = 1;
            GREEN = 0;
            RED = 0;
            break;
        }
        //Red LED, Drive like you stole it
        if( RPM >= (0.95 * MaxRPM))
        {
            RED = 1;
            BLUE = 0;
            GREEN = 0;
            break;
        }
    }
}
