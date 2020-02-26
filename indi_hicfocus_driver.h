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
#include "image.h"
#include "baseclient.h"
#include "defaultdevice.h"
#define NB_DEVICES 2


class HICFocuser : public INDI::DefaultDevice
{
  public:
    static const std::string DEVICE_NAME;
    HICFocuser();
    ~HICFocuser() = default;

    // DefaultDevice

    bool initProperties();
    bool updateProperties();
    void ISGetProperties(const char *dev);
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    bool ISSnoopDevice(XMLEle *root);

  protected:
    const char *getDefaultName();
    bool Connect();
    bool Disconnect();
    bool saveConfigItems(FILE *fp) override;

  private:
    bool isRunning();
    bool isCCDConnected();
    bool isFilterConnected();
    bool isTelescopeConnected();
    bool isGuideConnected();
    void defineProperties();
    void deleteProperties();
    void startFocus();
    void abortFocus();

    char *controlledCCD { nullptr };
    char *controlledFocuser { nullptr };
    HICImage CCDImage;

    ITextVectorProperty ControlledDeviceTP;
    IText ControlledDeviceT[NB_DEVICES];
    INumberVectorProperty GroupCountNP;
    INumber GroupCountN[1];
    INumberVectorProperty ProgressNP;
    INumber ProgressN[3];
    ISwitchVectorProperty BatchSP;
    ISwitch BatchS[2];
    ISwitchVectorProperty FocusSP;
    ISwitch FocusS[2];
    ILightVectorProperty StatusLP;
    ILight StatusL[NB_DEVICES];
    ITextVectorProperty ImageNameTP;
    IText ImageNameT[2];
    INumberVectorProperty DownloadNP;
    INumber DownloadN[2];
    IBLOBVectorProperty FitsBP;
    IBLOB FitsB[1];

    INumberVectorProperty FocusIMGNP;
    INumber FocusIMGN[5];
    INumberVectorProperty FocusEXPNP;
    INumber FocusEXPN[5];

    INumberVectorProperty CCDImageExposureNP;
    INumber CCDImageExposureN[1];
    INumberVectorProperty CCDImageBinNP;
    INumber CCDImageBinN[2];
    ISwitch CCDUploadS[3];
    ISwitchVectorProperty CCDUploadSP;
    IText CCDUploadSettingsT[2];
    ITextVectorProperty CCDUploadSettingsTP;

    INumberVectorProperty FilterSlotNP;
    INumber FilterSlotN[1];
    
};
