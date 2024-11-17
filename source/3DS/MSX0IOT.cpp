
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "3DSConfig.h"

#include "MSX.h"

#include <3ds.h>
#include "3DSLib.h"

#define IOT_NODE_NONE        0
#define IOT_NODE_ACCELX      1   /* accelerometer X axis    */
#define IOT_NODE_ACCELY      2   /* accelerometer Y axis    */
#define IOT_NODE_ACCELZ      3   /* accelerometer Z axis    */
#define IOT_NODE_ANALOG_IN   4   /* analog input            */
#define IOT_NODE_ANALOG_OUT  5   /* analog output           */
#define IOT_NODE_I2C_I       6   /* device find i2c_i       */
#define IOT_NODE_I2C_A       7   /* device find i2c_a       */
#define IOT_NODE_BATTERY     8   /* battery voltage         */
#define IOT_NODE_BATTERY_LV  9   /* battery level           */
#define IOT_NODE_WIFI_LV    10   /* wifi level              */
#define IOT_NODE_HEAP       11   /* heap memory             */
#define IOT_NODE_I2C_IN     12   /* device find i2c_i input */
#define IOT_NODE_CPU_LOAD   13   /* cpu usage percentage    */
#define IOT_READ_CHR    (IOTReadPos - (IOTReadPos / 3) - 1)

#define CHECK_IOT2(N, A, B)           ((IOTCmdData[N] == A) && (IOTCmdData[N + 1] == B))

#define CHECK_IOT3(N, A, B, C)        ((IOTCmdData[N] == A) && (IOTCmdData[N + 1] == B) && (IOTCmdData[N + 2] == C))

#define CHECK_IOT4(N, A, B, C, D)     ((IOTCmdData[N] == A) && (IOTCmdData[N + 1] == B) && (IOTCmdData[N + 2] == C)     \
            && (IOTCmdData[N+3] == D))

#define CHECK_IOT5(N, A, B, C, D, E)  ((IOTCmdData[N] == A) && (IOTCmdData[N + 1] == B) && (IOTCmdData[N + 2] == C)     \
            && (IOTCmdData[N + 3] == D) && (IOTCmdData[N + 4] == E))

#define CHECK_IOT6(N, A, B, C, D, E, F) ((IOTCmdData[N] == A) && (IOTCmdData[N + 1] == B) && (IOTCmdData[N + 2]==C)     \
            && (IOTCmdData[N + 3] == D) && (IOTCmdData[N +4] == E) && (IOTCmdData[N + 5] == F))

unsigned char UseMSX0 = 0;
unsigned char LoadXBASIC = 0;
int IOTMode = IOT_NODE_NONE;
unsigned char MSX0_I2CA = 1;
unsigned char MSX0_ANALOGOUT = 0;
unsigned char IOTVal;;
int IOTCmdPos = 0;
int IOTReadPos = 0;
int IOTPUTRestData = 0;
int IOTDisplayPos = 0;
unsigned char IOTCmdData[32];

#ifdef _3DS
unsigned char DeviceInited = 0;      /* 1:Gyroscope 2:MCUHWC*/
angularRate aRate;
#endif // _3DS


word InMSX0IOT()
{
    word Port;
    u32 cpuval = 0;
    char iotc[2];
    if (!UseMSX0)return NORAM;

    /* First command data contains "E0" if valid MSX0 data. */
    if (IOTCmdData[0] != 0xE0)return NORAM;

    /* 4th command data contais I/O direction. 0x80:Inputdata  0xC0:Oputput data. */
    if (IOTCmdData[3] != 0x80)return NORAM;

    if (Verbose & 0x20) printf("I/O: Read from MSX IOT Command\n");

    switch (IOTCmdData[2] & 0xF0)
    {
    case 0x00:  /* IOT Get */
        switch (IOTMode)
        {
        case IOT_NODE_NONE:   /* No IOT device. */
            break;
        case IOT_NODE_ACCELX:   /* Accel x (y for NINTENDO 3DS gyrometer) */
            Port = ReadIOTGET((ushort)(aRate.y >> 2));
            return Port;
        case IOT_NODE_ACCELY:   /* Accel y (x for NINTENDO 3DS gyrometer) */
            Port = ReadIOTGET((ushort)(aRate.x >> 2));
            return Port;
        case IOT_NODE_ACCELZ:   /* Accel z */
            Port = ReadIOTGET((ushort)(aRate.z >> 2));
            return Port;
        case IOT_NODE_ANALOG_IN:   /* Analog In */
            Port = (word)(osGet3DSliderState() * 4096.0);
            Port = Port > 4095 ? 4095 : Port; /* Upper 4bit ignored. */
            Port = ReadIOTGET(Port);
            return Port;
        case IOT_NODE_BATTERY:  /* Battery current */
            MCUHWC_GetBatteryVoltage(&IOTVal);
            Port = ReadIOTGET(IOTVal);
            return Port;

        case IOT_NODE_BATTERY_LV:   /* Battery level */
            MCUHWC_GetBatteryLevel(&IOTVal);
            Port = ReadIOTGET(IOTVal);
            return Port;

        case IOT_NODE_WIFI_LV:      /* Wifi strength level */
            return(ReadIOTGET(osGetWifiStrength()));

        case IOT_NODE_HEAP:        /* Heap memory */
            return (osGetMemRegionFree(MEMREGION_ALL) / 1000);

        case IOT_NODE_I2C_IN:
            return(ReadIOTGET(IOTVal));

        case IOT_NODE_CPU_LOAD:
            APT_GetAppCpuTimeLimit(&cpuval);
            Port = ReadIOTGET(cpuval);
            return Port;

        default:
            break;
        }
        break;

    case 0x10:  /* IOT Find */
        switch (IOTCmdData[2] & 0x0F)
        {
        case 1:     /* Integer(2byte)(length of the string) */
            switch (IOTReadPos)
            {
            case 0:
                IOTReadPos++;
                return(0);
            case 1:
                IOTReadPos++;
                return(0);
            case 2:
                switch (IOTMode)
                {
                case IOT_NODE_I2C_I:
                    Port = 5;
                    break;
                case IOT_NODE_I2C_A:
                    Port = !MSX0_I2CA ? 0 : 1;
                    break;
                default:
                    break;
                }
                IOTReadPos = 0;
                ResetIOTData();
                return Port;
            default:
                break;
            }
            break;
        case 3:     /* String */
            if (IOTMode == IOT_NODE_I2C_I)
            {
                switch (IOTReadPos)
                {
                case 0:
                case 3:
                case 6:
                case 9:
                case 12:
                    IOTReadPos++;
                    return(0);
                case 1:
                    IOTReadPos++; return(0x30);
                case 2:
                    IOTReadPos++; return(0x38);
                case 4:
                    IOTReadPos++; return(0x33);
                case 5:
                    IOTReadPos++; return(0x34);
                case 7:
                    IOTReadPos++; return(0x30);
                case 8:
                    IOTReadPos++; return(0x38);
                case 10:
                    IOTReadPos++; return(0x35);
                case 11:
                    IOTReadPos++; return(0x31);
                case 13:
                    IOTReadPos++; return(0x36);
                case 14:
                    IOTReadPos = 0;
                    ResetIOTData();
                    return(0x38);
                default:
                    break;
                }
            }
            else
            {
                switch (IOTReadPos)
                {
                case 0:
                    IOTReadPos++;
                    break;
                case 1:
                    switch (IOTMode)
                    {
                    case IOT_NODE_I2C_A:
                        Port = MSX0_I2CA;
                        break;
                    default:
                        break;
                    }
                    IOTReadPos++;
                    //if (Port == 1)return 0x34;
                    switch (Port)
                    {
                    case 1:
                        return 0x34;
                    case 2:     /* LCD */
                        return 0x33;
                    default:
                        break;
                    }
                case 2:
                    switch (IOTMode)
                    {
                    case IOT_NODE_I2C_A:
                        Port = MSX0_I2CA;
                        break;
                    default:
                        break;
                    }
                    IOTReadPos = 0;
                    ResetIOTData();
                    //if (Port == 1)return 0x30;
                    switch (Port)
                    {
                    case 1:
                        return 0x30;
                    case 2:     /* LCD "device/i2c_a/3E"*/
                        return 0x45;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
            return (0);
        default:
            break;
        }
        break;
    default:
        break;
    }
    return NORAM;
}


byte ReadIOTGET(int val)
{
    switch (IOTReadPos)
    {
    case 0:
        IOTReadPos++;
        return 2;
    case 1:
        IOTReadPos++;
        return (val & 0xFF);
    case 2:
        IOTReadPos = 0;
        ResetIOTData();
        return(val >> 8);
    default:
        break;
    }
}


void OutMSX0IOT(unsigned char val)
{
    /* MSX0 IOT command. */
/* Based on analization by uniskie. */
/* https://github.com/uniskie/MSX_MISC_TOOLS/tree/main/for_MSX0 */

    if (!UseMSX0)return;

    /* First command data contains "E0" if valid MSX0 data. */
    if (val == 0xE0)
    {
        IOTCmdData[0] = 0xE0;
        IOTCmdPos = 1;
        if (Verbose & 0x20)
            printf("I/O: Write to MSX IOT Command=%02Xh  IOTPos=0\n", val);
        return;
    }

    if (Verbose & 0x20)
        printf("I/O: Write to MSX IOT Command=%02Xh  IOTPos=%d\n", val, IOTCmdPos);

    if (IOTPUTRestData > 0)   /* IOTPUT Data remains */
    {
        if (((IOTCmdData[2] & 0xF0) == 0x40) && ((IOTCmdData[3] & 0xF0) == 0xC0))   /* IOTPUT Output */
        {
#ifdef MSX0_OLED
            if (MSX0_I2CA == 2) /* OLED Display */
            {
                switch (IOTCmdData[5])  /* I2C Control */
                {
                case 0x00:  /* Command data */
                    if (IOTCmdPos == 7)
                    {
                        switch (IOTCmdData[6])
                        {
                        case 0x00:   /* 0x00:set lower column address  */
                            IOTDisplayPos = IOTDisplayPos & 0xFF00 | val;
                            break;

                        case 0x10:  /* 0x10:set higher column address */
                            IOTDisplayPos = IOTDisplayPos & 0xFF | (val << 8);
                            break;

                        case 0xB0:  /* 0xB0:set page address */
                            IOTDisplayPos |= (val & 0x07) * 128 * 8;
                            break;
                        default:
                            break;
                        }
                    }
                    break;

                case 0x80:  /* Multi Command */
                    break;
                    //case 0x21:  /* Set Column Address */
                    //    IOTDisplayPos = val % 128;
                    //    break;
                    //case 0x22:  /* Set Page Address */
                    //    IOTDisplayPos |= (val&0x07) * 128;
                    //    break;
                case 0x40:  /* Display Data */
                    DrawOLED_Display(IOTDisplayPos, val);
                    IOTDisplayPos++;
                    //if (IOTDisplayPos > 0x800)IOTDisplayPos = 0;
                    break;
                default:
                    break;
                }
            }
#endif // MSX0_OLED

            IOTPUTRestData--;
            IOTCmdData[IOTCmdPos] = val;
            IOTCmdPos++;

            if (Verbose & 0x20)
            {
                printf("IOTPUT Output[%2Xh]\n", val);
                if (!IOTPUTRestData)printf("IOTPUT Send Data Finished\n");
            }
            return;
        }
        else IOTPUTRestData = 0;
    }

    switch (IOTCmdPos)
    {
    case 3:
        IOTCmdData[IOTCmdPos] = val;
        /* Upper 4 bit of 3rd command data contains IOT command signal. 0x00:_IOTGET  0x40:_IOTPUT  0x10:_IOTFIND */
        /* 4th command data contais I/O direction. 0x80:Inputdata  0xC0:Oputput data. */
        /* IOTGET Input */
        if ((val == 0x80) && ((IOTCmdData[2] & 0xF0) == 0x00))
        {
            /* "device" */
            if (IOTCmdData[5] == 'd' && IOTCmdData[6] == 'e' && IOTCmdData[7] == 'v' && IOTCmdData[8] == 'i' && IOTCmdData[9] == 'c' && IOTCmdData[10] == 'e')
            {
                /* "accel" */
                if (IOTCmdData[12] == 'a' && IOTCmdData[13] == 'c' && IOTCmdData[14] == 'c' && IOTCmdData[15] == 'e' && IOTCmdData[16] == 'l')
                {
#ifdef _3DS
                    if (!(DeviceInited & 0x01))
                    {
                        HIDUSER_EnableGyroscope();
                        DeviceInited |= 1;
                    }
                    hidGyroRead(&aRate);
                    if (Verbose & 0x20)
                        printf("accel X: %d    accel Y: %d    accel Z: %d \n", aRate.x, aRate.y, aRate.z);
#endif // _3DS
                    switch (IOTCmdData[18])
                    {
                    case 0x78:    /* "x" */
                        IOTMode = IOT_NODE_ACCELX;
                        IOTCmdPos = 0;
                        return;
                    case 0x79:    /* "y" */
                        IOTMode = IOT_NODE_ACCELY;
                        IOTCmdPos = 0;
                        return;
                    case 0x7A:    /* "z" */
                        IOTMode = IOT_NODE_ACCELZ;
                        IOTCmdPos = 0;
                        return;
                    default:
                        break;
                    }
                }
                /* "analog" */
                if (IOTCmdData[12] == 'a' && IOTCmdData[13] == 'n' && IOTCmdData[14] == 'a' && IOTCmdData[15] == 'l' && IOTCmdData[16] == 'o' && IOTCmdData[17] == 'g')
                {
                    /* "in" */
                    //if (IOTCmdData[19] == 'i' && IOTCmdData[20] == 'n')
                    if(CHECK_IOT2(19, 'i', 'n'))
                    {
                        IOTMode = IOT_NODE_ANALOG_IN;
                        IOTCmdPos = 0;
                        return;
                    }
                }
                /* i2c_i */
                if (IOTCmdData[12] == 'i' && IOTCmdData[13] == '2' && IOTCmdData[14] == 'c' && IOTCmdData[15] == '_' && IOTCmdData[16] == 'i')
                {
                    IOTMode = IOT_NODE_I2C_IN;
                    IOTCmdPos = 0;
                    return;
                }
            }
            /* "host" */
            //else if (IOTCmdData[5] == 'h' && IOTCmdData[6] == 'o' && IOTCmdData[7] == 's' && IOTCmdData[8] == 't')
            else if(CHECK_IOT4(5, 'h', 'o', 's', 't'))
            {
                /* "battery" */
                if (IOTCmdData[10] == 'b' && IOTCmdData[11] == 'a' && IOTCmdData[12] == 't' && IOTCmdData[13] == 't' && IOTCmdData[14] == 'e'
                    && IOTCmdData[15] == 'r' && IOTCmdData[16] == 'y')
                {
                    /* "current" */
                    if (IOTCmdData[18] == 'c' && IOTCmdData[19] == 'u' && IOTCmdData[20] == 'r' && IOTCmdData[21] == 'r' && IOTCmdData[22] == 'e'
                        && IOTCmdData[23] == 'n' && IOTCmdData[24] == 't')
                    {
                        if (!(DeviceInited & 0x02))
                        {
                            mcuHwcInit();
                            DeviceInited |= 0x02;
                        }
                        IOTMode = IOT_NODE_BATTERY;
                        IOTCmdPos = 0;
                        return;
                    }
                    if (IOTCmdData[18] == 'l' && IOTCmdData[19] == 'e' && IOTCmdData[20] == 'v' && IOTCmdData[21] == 'e' && IOTCmdData[22] == 'l')
                    {
                        if (!(DeviceInited & 0x02))
                        {
                            mcuHwcInit();
                            DeviceInited |= 0x02;
                        }
                        IOTMode = IOT_NODE_BATTERY_LV;
                        IOTCmdPos = 0;
                        return;
                    }
                }
                if (IOTCmdData[10] == 'w' && IOTCmdData[11] == 'i' && IOTCmdData[12] == 'f' && IOTCmdData[13] == 'i')
                {
                    if (IOTCmdData[15] == 'l' && IOTCmdData[16] == 'e' && IOTCmdData[17] == 'v' && IOTCmdData[18] == 'e' && IOTCmdData[19] == 'l')
                    {
                        IOTMode = IOT_NODE_WIFI_LV;
                        IOTCmdPos = 0;
                        return;
                    }
                }
                //if (IOTCmdData[10] == 'h' && IOTCmdData[11] == 'e' && IOTCmdData[12] == 'a' && IOTCmdData[13] == 'p')
                if(CHECK_IOT4(10, 'h', 'e', 'a', 'p'))
                {
                    IOTMode = IOT_NODE_HEAP;
                    IOTCmdPos = 0;
                    return;
                }
            }
            else if (IOTCmdData[5] == 'm' && IOTCmdData[6] == 's' && IOTCmdData[7]== 'x' && IOTCmdData[15] =='c' && IOTCmdData[16]=='p' && IOTCmdData[17]=='u')
            {
                if (IOTCmdData[19] == 'l' && IOTCmdData[20] == 'o' && IOTCmdData[21] == 'a' && IOTCmdData[22] == 'd')
                {
                    IOTMode = IOT_NODE_CPU_LOAD;
                    IOTCmdPos = 0;
                    return;
                }
            }
            IOTMode = 0;
            IOTCmdPos = 0;
            return;
        }
        /* IOTFIND Input */
        if ((val == 0x80) && ((IOTCmdData[2] & 0xF0) == 0x10))
        {
            /* "device" */
            if (IOTCmdData[5] == 'd' && IOTCmdData[6] == 'e' && IOTCmdData[7] == 'v' && IOTCmdData[8] == 'i' && IOTCmdData[9] == 'c' && IOTCmdData[10] == 'e')
            {
                if (IOTCmdData[12] == 'i' && IOTCmdData[13] == '2' && IOTCmdData[14] == 'c' && IOTCmdData[15] == '_')
                {
                    /* "i2c_i" */
                    if (IOTCmdData[16] == 'i')
                    {
                        IOTMode = IOT_NODE_I2C_I;
                        IOTCmdPos = 0;
                        return;
                    }
                    /* "i2c_a" */
                    if (IOTCmdData[16] == 'a')
                    {
                        IOTMode = IOT_NODE_I2C_A;
                        IOTCmdPos = 0;
                        return;
                    }
                }
            }
            IOTMode = 0;
            IOTCmdPos = 0;
            return;
        }
        break;

    case 4:
        if (((IOTCmdData[2] & 0xF0) == 0x40) && ((IOTCmdData[3] & 0xF0) == 0xC0))   /* IOTPUT Output */
        {
            IOTPUTRestData = val;
            if (Verbose & 0x20)
                printf("IOTPUT Send Data Start. Length[%d]\n", val);
        }
        break;

#ifdef MSX0_OLED
    case 5:
        if (MSX0_I2CA == 2)     /* 2:OLED Display   */
        {
            switch (val)
            {
            case 0x00:
                IOTDisplayPos = 0;
                break;
            case 0x40:      /* 0x40:i2c_a send display data start command. */
                break;
            default:
                break;
            }
        }
        break;
#endif // MSX0_OLED

    case 6:
        /* IOTPUT Output */
        if ((IOTCmdData[2] & 0xF0) == 0x40)
        {
            /* analog */
            if (IOTCmdData[12] == 'a' && IOTCmdData[13] == 'n' && IOTCmdData[14] == 'a' && IOTCmdData[15] == 'l' && IOTCmdData[16] == 'o' && IOTCmdData[17] == 'g')
            {
                /* "out" */
                if (IOTCmdData[19] == 'o' && IOTCmdData[20] == 'u' && IOTCmdData[21] == 't')
                {
                    if (MSX0_ANALOGOUT == 1)    /* LED(Analog ooutput) */
                    {
                        /* LED with 3DS's Power LED(Unsafe!) */
                        if (!(DeviceInited & 0x02))
                        {
                            mcuHwcInit();
                            DeviceInited |= 0x02;
                        }
                        unsigned char ledval = (((word)(val & 0x0F) << 8 | (word)IOTCmdData[5])) >> 4;
                        MCUHWC_WriteRegister(0x28, &ledval, 1);    // i2c Register 0x28 for 3DS 	Brightness of the WiFi/Power LED
                                                                    //https://www.3dbrew.org/wiki/I2C_Registers#Device_3
                    }
                    if (Verbose & 0x20)
                        printf("Analog Out[%d]\n", val << 8 | IOTCmdData[5]);
                    IOTCmdPos = 0;
                    return;
                }
            }
            else if (IOTCmdData[5] == 0x94) /* AXP192 GPIO1 control register 0x94. MSX0 Power Indicator LED */
            {
                if (Verbose & 0x20)
                    printf("IOTPUT AXP192[%d]\n", val);
                /* LED with 3DS's Power LED(Unsafe!) */
                if (!(DeviceInited & 0x02))
                {
                    mcuHwcInit();
                    DeviceInited |= 0x02;
                }
                IOTVal = val;
                /* Info from Ninune-wa. */
                /* https://github.com/Ninune-wa/MSX0-Sensor-Utility/tree/main/MSX0Stack-LED(PowerIndicatorLight) */
                /* GPIO1 bit1 1:disable 0:enable */
                if (val & 0x02)MCUHWC_SetPowerLedState(LED_OFF);
                else MCUHWC_SetPowerLedState(LED_BLUE);
                IOTCmdPos = 0;
                return;
            }
            if (Verbose & 0x20)
                printf("IOTPUT to Unkown module[%2Xh]\n", val);
        }
        break;
    default:
        break;
    }
    IOTCmdData[IOTCmdPos] = val;
    IOTCmdPos++;
    return;
}


void ResetIOTData()
{
    byte J;
    for (J = 0; J < 32; J++)IOTCmdData[J] = 0;
    IOTCmdPos = 0;
    IOTMode = 0;
    IOTPUTRestData = 0;
}

void TrashMSX0()
{
    if (DeviceInited & 0x01)HIDUSER_DisableGyroscope();
    if (DeviceInited & 0x02)mcuHwcExit();
}