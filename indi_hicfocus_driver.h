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

#pragma once

#include "config.h"
#include "defaultdevice.h"
#include "indi_hicfocus_client.h"
#define NB_DEVICES 2


class HICFocuser : public INDI::DefaultDevice
{
  public:
    static const std::string DEVICE_NAME;
    HICFocuser();
    ~HICFocuser() = default;

    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

private:
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool Connect();
    virtual bool Disconnect() ;
    virtual bool saveConfigItems(FILE *fp) ;

    bool isRunning();
    bool isCCDConnected();
    bool isFilterConnected();
    bool isTelescopeConnected();
    bool isGuideConnected();
    void startFocus();
    void abortFocus();

/*    char *controlledCCD { nullptr };
    char *controlledFocuser { nullptr };
    HICImage CCDImage;*/
};
