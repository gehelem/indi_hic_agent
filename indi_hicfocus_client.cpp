#include "indi_hicfocus_client.h"
#include "polynomialfit.h"
#include "defaultdevice.h"
#include "image.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <cstdlib>

#define MYCCD "CCD Simulator"

static std::unique_ptr<MyClient> camera_client(new MyClient());
bool HICClientRunning;
int posmin,posmax,postarget,steps,stage;
INumberVectorProperty *ccd_exposure = nullptr;
INumberVectorProperty *focuser_pos = nullptr;
double chisq;
double hfdtarget;


char *clientCamera,*clientFocuser,*clientTelescope;
HICImage cameraImage;


struct focuspointT {
  double pos;
  double hfd;
};

typedef std::vector<focuspointT> focuscurveT;
focuspointT focuspoint;
focuscurveT focuscurve;
std::vector<double> posvector;
std::vector<double> hfdvector;
std::vector<double> coefficients;

void HICClientStart(void)
{
    IDLog("HICCLientStart\n");
    HICClientRunning = true;
    camera_client->setServer("localhost",7624);
    //camera_client->watchDevice("HIC Focuser");
    clientCamera=nullptr;
    clientFocuser=nullptr;
    camera_client->connectServer();
    std::thread t1 (&HICClientThread,nullptr);
    t1.detach();
    /*IDLog("HICCLientStart\n");
    HICClientRunning = true;
    posmin=min;
    posmax=max;
    steps=step;
    clientCamera=camera;
    clientFocuser=focuser;
    postarget=min;
    stage=0;
    hfdtarget=0;
    focuscurve.erase(focuscurve.begin(),focuscurve.end());
    posvector.erase(posvector.begin(),posvector.end());
    hfdvector.erase(hfdvector.begin(),hfdvector.end());
    camera_client->setServer(hostname,port);
    camera_client->watchDevice(clientCamera);
    camera_client->watchDevice(clientFocuser);
    camera_client->watchDevice("HIC Focuser");
    camera_client->connectServer();
    camera_client->setBLOBMode(B_ALSO, clientCamera, nullptr);

    std::thread t1 (&HICClientThread,nullptr);
    t1.detach();*/
}
void HICClientThread(void *arg)
{
    IDLog("HICCLientStart--thread\n");

    while(HICClientRunning){
            sleep(1);
    };
    camera_client->disconnectServer();
            IDLog("Exit Thread\n");
    ( void ) arg;
    pthread_exit ( NULL );
}
void HICClientStop(void)
{
    HICClientRunning = false;
}
/**************************************************************************************
**
***************************************************************************************/
MyClient::MyClient()
{
    HICDevice = nullptr;
    cameraDevice = nullptr;
    focuserDevice = nullptr;
}



/**************************************************************************************
**
***************************************************************************************/
void MyClient::newDevice(INDI::BaseDevice *dp)
{
    /*if (strcmp(dp->getDeviceName(), clientCamera) == 0) {
        IDLog("Receiving device %s ...\n", dp->getDeviceName());
        //cameraDevice = dp;
    }
    if (strcmp(dp->getDeviceName(), clientFocuser) == 0) {
        IDLog("Receiving device %s ...\n", dp->getDeviceName());
        //focuserDevice = dp;
    }*/
    if (strcmp(dp->getDeviceName(), "HIC Focuser") == 0) {
        IDLog("Receiving device %s ...\n", dp->getDeviceName());
        HICDevice = dp;
    }
}

/**************************************************************************************
**
*************************************************************************************/
void MyClient::newProperty(INDI::Property *property)
{
    IDLog("Client receiving new property %s %s...\n",property->getDeviceName(), property->getName());

    if (strcmp(property->getDeviceName(), "HIC Focuser") == 0 && strcmp(property->getName(), "HICFOCUS_DEVICES") == 0)
    {
        //connectDevice(property->getDeviceName());
        IDLog("Connecting %s\n",property->getDeviceName());
        ITextVectorProperty *tvp = property->getText();
        clientTelescope = tvp->tp[0].text;
        clientCamera = tvp->tp[1].text;
        clientFocuser = tvp->tp[2].text;

        telescopeDevice = getDevice(clientTelescope);
        cameraDevice = getDevice(clientCamera);
        focuserDevice = getDevice(clientFocuser);
        connectDevice(clientCamera);
        connectDevice(clientFocuser);
        connectDevice(clientTelescope);
        setBLOBMode(B_ALSO, clientCamera, nullptr);

        /*watchDevice(HICDevice->getDeviceName());
        watchDevice(clientCamera);
        watchDevice(clientFocuser);
        watchProperty(clientTelescope,"CONNECTION");*/


        return;
    }
    /*if (strcmp(property->getDeviceName(), clientFocuser) == 0 && strcmp(property->getName(), "ABS_FOCUS_POSITION") == 0)
    {
        INumberVectorProperty *focuser_pos = nullptr;
        focuser_pos = focuserDevice->getNumber("ABS_FOCUS_POSITION");
        focuser_pos->np[0].value = postarget;
        sendNewNumber(focuser_pos);
        return;
    }*/

}
/**************************************************************************************
**
***************************************************************************************/
void MyClient::newSwitch(ISwitchVectorProperty *svp)
{
    IDLog("Client receiving new switch %s %s ...\n", svp->device,svp->name);

    if (strcmp(svp->device, "HIC Focuser" ) == 0 && strcmp(svp->name, "CONNECTION") == 0)
    {
        //IDLog("connection %s %i ...\n", svp->sp[0].name,svp->sp[0].s);
        //IDLog("connection %s %i ...\n", svp->sp[1].name,svp->sp[1].s);
        if (svp->sp[1].s==ISS_ON) HICClientRunning=false;
    }

}
/**************************************************************************************
**
***************************************************************************************/
void MyClient::newLight(ILightVectorProperty *lvp)
{
    IDLog("Client receiving new light %s ...\n", lvp->name);

    if (strcmp(lvp->device, "HIC Focuser" ) == 0 && strcmp(lvp->name, "xxx") == 0)
    {
        // do stuff
    }

}


/**************************************************************************************
**
***************************************************************************************/
void MyClient::newText(ITextVectorProperty *tvp)
{
    IDLog("Client receiving new text %s ...\n", tvp->name);

    if (strcmp(tvp->device, "HIC Focuser" ) == 0 && strcmp(tvp->name, "CONNECTION") == 0)
    {
        //do stuff
    }

}
/**************************************************************************************
**
***************************************************************************************/
void MyClient::newNumber(INumberVectorProperty *nvp)
{
    IDLog("Client receiving new number %s ...\n", nvp->name);

    /*if (strcmp(nvp->device, clientFocuser) == 0 && strcmp(nvp->name, "ABS_FOCUS_POSITION") == 0)
    {
        if (nvp->np[0].value == postarget)
        {
            //IDLog("focuser reached target %i\n", postarget);
            INumberVectorProperty *camera_exposure = nullptr;
            camera_exposure = cameraDevice->getNumber("CCD_EXPOSURE");
            camera_exposure->np[0].value = 100;
            sendNewNumber(camera_exposure);
            if (stage==1) stage=2;
        }

    }*/
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
    if (strcmp(dp->getDeviceName(), MYCCD) != 0)
        return;

    //IDLog("Client receiving message from Server:%s\n", dp->messageQueue(messageID).c_str());
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newBLOB(IBLOB *bp)
{

    cameraImage.LoadFromBlob(bp);
    IDLog("POS=%i HFD=%f MED=%f\n", postarget, cameraImage.hfd,cameraImage.med);

    posvector.push_back((double)postarget);
    hfdvector.push_back((double)cameraImage.hfd);

    postarget=postarget+steps;
    if (postarget < posmax && stage == 0 )
    {
        INumberVectorProperty *focuser_pos = nullptr;
        focuser_pos = focuserDevice->getNumber("ABS_FOCUS_POSITION");
        focuser_pos->np[0].value = postarget;
        sendNewNumber(focuser_pos);
    }
    if (postarget >= posmax && stage == 0 )
    {
        // find minimal
        stage =1;
        chisq = 0;
        /*coefficients = gsl_polynomial_fit(posvector.data(), hfdvector.data(), posvector.size(),3,chisq);
        IDLog("Coefs %lf %lf %lf\n",coefficients[0],coefficients[1],coefficients[2]);
        IDLog("chisq %lf\n",chisq);*/
        for (int i=0;i<posvector.size();i++)
        {
            IDLog("Values %lf %lf\n",posvector.data()[i],hfdvector.data()[i]);
        }


        #define DEGREE 3
        double coeff[DEGREE];
        polynomialfit(posvector.size(), DEGREE, posvector.data(), hfdvector.data(), coeff);
        for(int i=0; i < DEGREE; i++)
        {
          IDLog( "coefs %lf\n", coeff[i]*1000000);
        }

        postarget = -coeff[1]/(2*coeff[2]);
        hfdtarget = coeff[0]+coeff[1]*postarget+coeff[2]*postarget*postarget;
        IDLog( "Target position = %i\n", postarget);
        IDLog( "Target HFD = %lf\n", hfdtarget);

        INumberVectorProperty *focuser_pos = nullptr;
        focuser_pos = focuserDevice->getNumber("ABS_FOCUS_POSITION");
        focuser_pos->np[0].value = postarget;
        sendNewNumber(focuser_pos);


    }

}


