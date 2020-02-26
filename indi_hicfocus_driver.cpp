/*******************************************************************************
 HIC Agent - Headless Indi Controller Agent
 Copyright (C) 2020 - Gilles Le Maréchal (Gehelem)
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include "image.h"
#include "indi_hicfocus_driver.h"
#include "indistandardproperty.h"
#include "indi_hicfocus_client.h"


#include <cstring>
#include <algorithm>
#include <memory>


const std::string HICFocuser::DEVICE_NAME = "HIC Focuser";
std::shared_ptr<HICFocuser> hicfocuser(new HICFocuser());



// Driver entry points ----------------------------------------------------------------------------

void ISGetProperties(const char *dev)
{
    hicfocuser->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    hicfocuser->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    hicfocuser->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    hicfocuser->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    hicfocuser->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    hicfocuser->ISSnoopDevice(root);
}

// HICFocuser ----------------------------------------------------------------------------

HICFocuser::HICFocuser()
{
    setVersion(1, 2);
    //groups.resize(MAX_GROUP_COUNT);
    //int i=0;
    //std::generate(groups.begin(), groups.end(), [this, &i] { return std::make_shared<Group>(i++, this); });
}

bool HICFocuser::isRunning()
{
    return ProgressNP.s == IPS_BUSY;
}


void HICFocuser::startFocus()
{
    LOG_INFO("Focus started");
    //std::thread t1 (&HICAgent::focusingThread,this,nullptr);
    //t1.detach();
    HICClientStart("localhost",7624,controlledCCD,controlledFocuser,FocusEXPN[1].value,FocusEXPN[2].value,FocusEXPN[3].value,FocusEXPN[0].value);

}

void HICFocuser::abortFocus()
{
    /*ProgressNP.s = IPS_ALERT;
    IDSetNumber(&ProgressNP, "Focus aborted");*/
    LOG_INFO("Focus aborted");
    HICClientStop();
}

// DefaultDevice ----------------------------------------------------------------------------

const char *HICFocuser::getDefaultName()
{
    return HICFocuser::DEVICE_NAME.c_str();
}

bool HICFocuser::initProperties()
{
    INDI::DefaultDevice::initProperties();

    addDebugControl();

    IUFillText(&ControlledDeviceT[0], "CCD", "CCD", "CCD Simulator");
    IUFillText(&ControlledDeviceT[1], "FOCUS", "Focuser", "Focuser Simulator");
    IUFillTextVector(&ControlledDeviceTP, ControlledDeviceT, NB_DEVICES, getDefaultName(), "DEVICES", "Controlled devices",
                     MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);
    controlledCCD         = ControlledDeviceT[0].text;
    controlledFocuser     = ControlledDeviceT[1].text;

    IUFillLight(&StatusL[0], "CCD", controlledCCD, IPS_IDLE);
    IUFillLight(&StatusL[1], "FOCUS", controlledFocuser, IPS_IDLE);
    IUFillLightVector(&StatusLP, StatusL, NB_DEVICES, getDefaultName(), "STATUS", "Controlled devices", MAIN_CONTROL_TAB, IPS_IDLE);


    IUFillSwitch(&FocusS[0], "START_FOCUS", "Start focus", ISS_OFF);
    IUFillSwitch(&FocusS[1], "ABORT_FOCUS", "Abort focus", ISS_OFF);
    IUFillSwitchVector(&FocusSP, FocusS, 2, getDefaultName(), "BATCH_FOCUS", "Focus control", FOCUS_TAB, IP_RW, ISR_NOFMANY,
                       60, IPS_IDLE);

    IUFillNumber(&FocusIMGN[0], "FOCUS_IMG_HFD", "HFD", "%5.2f", -1,100,0,-1);
    IUFillNumber(&FocusIMGN[1], "FOCUS_IMG_MIN", "Min", "%5.0f", -1,100,0,-1);
    IUFillNumber(&FocusIMGN[2], "FOCUS_IMG_MAX", "Max", "%5.0f", -1,100,0,-1);
    IUFillNumber(&FocusIMGN[3], "FOCUS_IMG_AVG", "Average", "%5.2f", -1,100,0,-1);
    IUFillNumber(&FocusIMGN[4], "FOCUS_IMG_MED", "Median", "%5.2f", -1,100,0,-1);
    IUFillNumberVector(&FocusIMGNP, FocusIMGN, 5, getDefaultName(), "FOCUS_IMG", "Image statistics", FOCUS_TAB, IP_RO,
                       60, IPS_IDLE);

    IUFillNumber(&FocusEXPN[0], "FOCUS_EXP_DURATION", "Duration (s)", "%5.2f", 1,36000,0,100);
    IUFillNumber(&FocusEXPN[1], "FOCUS_POS_MIN", "Min position", "%5f",0,100000,100,20000);
    IUFillNumber(&FocusEXPN[2], "FOCUS_POS_MAX", "Max position", "%5f",0,100000,100,50000);
    IUFillNumber(&FocusEXPN[3], "FOCUS_POS_STEPS", "Steps", "%5f",0,100000,100,1000);
    IUFillNumberVector(&FocusEXPNP, FocusEXPN, 5, getDefaultName(), "FOCUS_EXP", "Image exposure", FOCUS_TAB, IP_RW,
                       60, IPS_IDLE);

    defineNumber(&GroupCountNP);
    defineText(&ControlledDeviceTP);
    defineNumber(&FocusEXPNP);
    defineText(&ImageNameTP);

    IUFillNumber(&CCDImageExposureN[0], "CCD_EXPOSURE_VALUE", "Duration (s)", "%5.2f", 0, 36000, 0, 1.0);
    IUFillNumberVector(&CCDImageExposureNP, CCDImageExposureN, 1, ControlledDeviceT[0].text, "CCD_EXPOSURE", "Expose",
                       MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);





    return true;
}

bool HICFocuser::updateProperties()
{
    if (isConnected())
    {
        defineLight(&StatusLP);
        defineNumber(&ProgressNP);
        BatchSP.s = IPS_IDLE;
        defineSwitch(&BatchSP);
        FocusSP.s = IPS_IDLE;
        defineSwitch(&FocusSP);
        DownloadN[0].value = 0;
        DownloadN[1].value = 0;
        DownloadNP.s       = IPS_IDLE;
        defineNumber(&DownloadNP);
        FitsBP.s = IPS_IDLE;
        defineBLOB(&FitsBP);
        defineNumber(&FocusEXPNP);
        defineNumber(&FocusIMGNP);
    }
    else
    {
        deleteProperty(StatusLP.name);
        deleteProperty(ProgressNP.name);
        deleteProperty(BatchSP.name);
        deleteProperty(FocusSP.name);
        deleteProperty(DownloadNP.name);
        deleteProperty(FitsBP.name);
        deleteProperty(FocusIMGNP.name);
        deleteProperty(FocusEXPNP.name);
    }
    return true;
}

void HICFocuser::ISGetProperties(const char *dev)
{
    DefaultDevice::ISGetProperties(dev);
}

bool HICFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (HICFocuser::DEVICE_NAME == dev)
    {
        if (std::string{name} == std::string{"dummy GroupCountNP.name"})
        {
            // do stuff
            return true;
        }
    }
    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool HICFocuser::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (HICFocuser::DEVICE_NAME == dev)
    {
        if (std::string{name} == std::string{FocusSP.name})
        {
            for (int i = 0; i < n; i++)
            {
                if (strcmp(names[i], FocusS[0].name) == 0 && states[i] == ISS_ON)
                {
                    //if (!isRunning())
                        startFocus();
                }
                if (strcmp(names[i], FocusS[1].name) == 0 && states[i] == ISS_ON)
                {
                    //if (isRunning())
                        abortFocus();
                }
            }
            FocusSP.s = IPS_OK;
            IDSetSwitch(&FocusSP, nullptr);
            return true;
        }
    }
    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool HICFocuser::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (HICFocuser::DEVICE_NAME == dev)
    {
        if (std::string{name} == std::string{ControlledDeviceTP.name})
        {
            IUUpdateText(&ControlledDeviceTP, texts, names, n);
            IDSetText(&ControlledDeviceTP, nullptr);
            strncpy(StatusL[0].label, ControlledDeviceT[0].text, sizeof(StatusL[0].label));
            strncpy(CCDImageExposureNP.device, ControlledDeviceT[0].text, sizeof(CCDImageExposureNP.device));
            strncpy(FocusEXPNP.device, ControlledDeviceT[0].text, sizeof(FocusEXPNP.device));
            strncpy(CCDImageBinNP.device, ControlledDeviceT[0].text, sizeof(CCDImageBinNP.device));
            strncpy(StatusL[1].label, ControlledDeviceT[1].text, sizeof(StatusL[1].label));
            strncpy(FilterSlotNP.device, ControlledDeviceT[1].text, sizeof(FilterSlotNP.device));
            strncpy(StatusL[2].label, ControlledDeviceT[2].text, sizeof(StatusL[2].label));
            strncpy(StatusL[3].label, ControlledDeviceT[3].text, sizeof(StatusL[3].label));
            strncpy(StatusL[4].label, ControlledDeviceT[4].text, sizeof(StatusL[4].label));
            strncpy(StatusL[5].label, ControlledDeviceT[5].text, sizeof(StatusL[5].label));
            return true;
        }
        if (std::string{name} == std::string{ImageNameTP.name})
        {
            IUUpdateText(&ImageNameTP, texts, names, n);
            IDSetText(&ImageNameTP, nullptr);
            return true;
        }
    }
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool HICFocuser::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
                       char *names[], int n)
{
    return INDI::DefaultDevice::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool HICFocuser::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool HICFocuser::Connect()
{
    return true;
}

bool HICFocuser::Disconnect()
{
    return true;
}
bool HICFocuser::saveConfigItems(FILE * fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);
    IUSaveConfigText(fp,&ControlledDeviceTP);
    return true;
}
