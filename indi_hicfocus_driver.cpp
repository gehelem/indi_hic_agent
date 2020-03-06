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



#include <cstring>
#include <algorithm>
#include <memory>


const std::string HICFocuser::DEVICE_NAME = "HIC Focuser";
std::shared_ptr<HICFocuser> hicfocuser(new HICFocuser());

//static std::unique_ptr<MyClient> hicclient(new MyClient());


// Driver entry points ----------------------------------------------------------------------------

void ISGetProperties(const char *dev) {    hicfocuser->ISGetProperties(dev);}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {    hicfocuser->ISNewSwitch(dev, name, states, names, n);}
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) {    hicfocuser->ISNewText(dev, name, texts, names, n);}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n){    hicfocuser->ISNewNumber(dev, name, values, names, n);}
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n) { hicfocuser->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);}
void ISSnoopDevice(XMLEle *root){ hicfocuser->ISSnoopDevice(root); }

// HICFocuser ----------------------------------------------------------------------------

HICFocuser::HICFocuser()
{
    setVersion( HICFOCUSAGENT_VERSION_MAJOR , HICFOCUSAGENT_VERSION_MINOR);

}

bool HICFocuser::isRunning()
{
    //return ProgressNP.s == IPS_BUSY;
    return true;
}


void HICFocuser::startFocus()
{
    LOG_INFO("Focus started");
    //std::thread t1 (&HICAgent::focusingThread,this,nullptr);
    //t1.detach();
    //HICClientStart("localhost",7624,controlledCCD,controlledFocuser,FocusEXPN[1].value,FocusEXPN[2].value,FocusEXPN[3].value,FocusEXPN[0].value);

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
    DefaultDevice::initProperties();
    const char *skelFileName = "/home/gilles/indi_hic_agent/HICFocus_sk.xml";
    struct stat st;
    char *skel = getenv("INDISKEL");

    if (skel != nullptr)
    {
        if (!buildSkeleton(skel)) LOG_WARN("Error building skeleton properties\n");
    }
    else if (stat(skelFileName, &st) == 0)
    {
        if (!buildSkeleton(skelFileName)) LOG_WARN("Error building skeleton properties\n");
    }
    else
    {
        IDLog("No skeleton file was specified. Set environment variable INDISKEL to the skeleton path and try again.\n");
    }

    /*addConfigurationControl();
    addAuxControls();
    addDebugControl();
    addSimulationControl();
    addPollPeriodControl();*/
    return true;

}

bool HICFocuser::updateProperties()
{
    if (!isConnected())
    {
    }

    return DefaultDevice::updateProperties();

}

void HICFocuser::ISGetProperties(const char *dev)
{
    static int configLoaded = 0;

    // Ask the default driver first to send properties.
    INDI::DefaultDevice::ISGetProperties(dev);

    // If no configuration is load before, then load it now.
    if (configLoaded == 0)
    {
        loadConfig();
        configLoaded = 1;
    }
}

bool HICFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //**************
    // Ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    INumberVectorProperty *nvp = getNumber(name);

    if (nvp == nullptr)
        return false;

    if (!isConnected())
    {
        nvp->s = IPS_ALERT;
        IDSetNumber(nvp, "Cannot change property while device is disconnected.");
        LOG_INFO("Cannot change property while device is disconnected.");

        return false;
    }
    //**************


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
    LOGF_INFO("Driver new switch %s %s\n",dev,name);
    //**************
    // ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    if (INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n))
        return true;

    ISwitchVectorProperty *svp = getSwitch(name);

    if (!isConnected())
    {
        svp->s = IPS_ALERT;
        IDSetSwitch(svp, "Cannot change property while device is disconnected.");
        LOG_INFO("Cannot change property while device is disconnected.");
        return false;
    }

    if (svp == nullptr)
        return false;
    //**************


    LOGF_INFO("Driver new switch %s %s\n",dev,name);

    if (HICFocuser::DEVICE_NAME == dev)
    {
        IDSetSwitch(svp,nullptr);
        return true;
        /*if (std::string{name} == std::string{FocusSP.name})
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
        }*/
    }
    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool HICFocuser::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //**************
    // ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    if (INDI::DefaultDevice::ISNewText(dev, name, texts, names, n))
        return true;

    ITextVectorProperty *tvp = getText(name);

    if (!isConnected())
    {
        tvp->s = IPS_ALERT;
        IDSetText(tvp, "Cannot change property while device is disconnected.");
        LOG_INFO("Cannot change property while device is disconnected.");
        return false;
    }

    if (tvp == nullptr)
        return false;
    //**************

    if (HICFocuser::DEVICE_NAME == dev)
    {
        /*if (std::string{name} == std::string{ControlledDeviceTP.name})
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
        }*/
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
    LOG_INFO("Connect()");
    HICClientStart();
    return true;
}

bool HICFocuser::Disconnect()
{
    LOG_INFO("DisConnect()");
    return true;
}

bool HICFocuser::saveConfigItems(FILE * fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);
    //IUSaveConfigText(fp,&ControlledDeviceTP);
    return true;
}
