/*******************************************************************************
 HIC Agent - Headless Indi Controller Agent
 Copyright (C) 2020 - Gilles Le Maréchal (Gehelem)
 Based on indi "Agent Imager" driver
 
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
#define MAX_GROUP_COUNT 16
#define NB_DEVICES 6


class Group;
class HICAgent : public virtual INDI::DefaultDevice, public virtual INDI::BaseClient
{
  public:
    static const std::string DEVICE_NAME;
    HICAgent();
    virtual ~HICAgent() = default;

    // DefaultDevice

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

    // BaseClient

    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property);
    virtual void removeDevice(INDI::BaseDevice *dp);
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newText(ITextVectorProperty *tvp);
    virtual void newLight(ILightVectorProperty *lvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);

  protected:
    virtual const char *getDefaultName();
    virtual bool Connect();
    virtual bool Disconnect();

  private:
    bool isRunning();
    bool isCCDConnected();
    bool isFilterConnected();
    bool isTelescopeConnected();
    bool isGuideConnected();
    void defineProperties();
    void deleteProperties();
    void initiateNextFilter();
    void initiateNextCapture();
    void startBatch();
    void abortBatch();
    void batchDone();
    void startFocus();
    void abortFocus();
    void initiateDownload();

    char format[16];
    int group { 0 };
    int maxGroup { 0 };
    int image { 0 };
    int maxImage { 0 };
    char *controlledCCD { nullptr };
    char *controlledTelescope { nullptr };
    char *controlledFocuser { nullptr };
    char *controlledFilterWheel { nullptr };
    char *controlledGPS { nullptr };
    char *controlledGuide { nullptr };
    HICImage CCDImage;
    HICImage GuideImage;

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

    std::vector<std::shared_ptr<Group>> groups;
    std::shared_ptr<Group> currentGroup() const;
    std::shared_ptr<Group> nextGroup() const;
    std::shared_ptr<Group> getGroup(int index) const;
    
};
