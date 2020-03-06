/*
    Tutorial Client
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "baseclient.h"
#include "image.h"



void HICClientStart(void);
void HICClientThread(void *arg);
void HICClientStop(void);


class MyClient : public INDI::BaseClient
{
  public:
    MyClient();
    ~MyClient() = default;

  protected:
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void removeDevice(INDI::BaseDevice */*dp*/) {}
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property */*property*/) {}
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *lvp);
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void newText(ITextVectorProperty *tvp);
    virtual void newLight(ILightVectorProperty *lvp);
    virtual void serverConnected() {}
    virtual void serverDisconnected(int /*exit_code*/) {}

  private:
    INDI::BaseDevice *HICDevice;
    INDI::BaseDevice *cameraDevice;
    INDI::BaseDevice *focuserDevice;
    INDI::BaseDevice *telescopeDevice;
    HICImage CCDImage;
};
