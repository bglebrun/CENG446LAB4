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

static void  upShift(void *data);
static void  downShift(void *data);
static void  autoShift(void *data);
static void  manualShift(void *data);
static void  modeSwitch(void);
static void  shiftLight(void *data);


//Data structure to hold all info passed from PC
struct Data
{
	float RPM;
    float MaxRPM;
	int GearCurrent;
	int GearMax;
	float Throttle;
};

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

    App_TaskCreate();                                           /* Create Application tasks                             */

    App_ObjCreate();                                            /* Create Applicaiton kernel objects                    */

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        calcVals();
        OSTimeDlyHMSM(0u, 0u, 0, 100u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
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
            (OS_PRIO )5,
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
            (OS_PRIO )5,
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
            (OS_PRIO )3,
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
            (OS_PRIO )6,
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
            //After mode toggled return to pending state
            break;
        }
        if( mode == 1 )
        {
            mode = 0;
            //After mode toggled return to pending state
            break;
        }
    } 
}

//Transmission Functions
static void  autoShift(void *data)
{
    //Create Structure to hold engine/gear data
    struct Data car;
    OS_ERR err;
    CPU_TS ts;
    
    while(1)
    {
        //Get data for RPM and current gear
        //car.RPM = ;
        //car.GearCurrent = ;
        
        //If manual mode activated pend and allow scheduler to go to manual mode
        if(mode == 1)
        {
            OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
        }
        //If RPM is within 5% of redline shift up
        if(car.RPM >= (0.95*car.MaxRPM))
        {
            upShift();
            break;
        }
        //If RPM is falling shift to lower gear
        if(car.RPM <= 1350)
        {
            downShift();
            break;
        }
        //Now send the data to the shiftLight task and pend
        shiftLight(&car);
        OSTaskSemPend(10000, OS_OPT_PEND_BLOCKING, &ts, &err);
        
    }
}
    
//Task to shift using the paddle shifters
static void  manualShift(void *data)
{
    OS_ERR err;
    CPU_TS ts;
    
    //Pend waiting for autoShift to go to sleep
    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
    while(1)
    {
       if(mode == 0)
       {
           OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, &ts, &err);
       }
           
    }
}

//Function to shift up one gear
static void  upShift(void)
{
    
}

//Function to shift down one gear
static void  downShift(void)
{
    
}

static void shiftLight(void *data)
{
    //Output to shift light and gear display
    
    
    
}
