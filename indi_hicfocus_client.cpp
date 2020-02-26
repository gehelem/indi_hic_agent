#include "indi_hicfocus_client.h"
#include "polynomialfit.h"
#include "defaultdevice.h"

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


char *clientCamera,*clientFocuser;
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

void HICClientStart(const char *hostname, unsigned int port, char *camera,char *focuser,int min, int max, int step, int sec)
{
    IDLog("HICCLientStart\n");
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
    camera_client->connectServer();
    camera_client->setBLOBMode(B_ALSO, clientCamera, nullptr);

    std::thread t1 (&HICClientThread,nullptr);
    t1.detach();
}
void HICClientStop(void)
{
    HICClientRunning = false;
}
void HICClientThread(void *arg)
{
    IDLog("HICCLientStart\n");
    /*sleep(5);
    IDLog("--1");
    ccdDevice = camera_client->getDevice(DeviceCCD);
    IDLog("--2");
    focuserDevice = camera_client->getDevice(DeviceFocuser);
    IDLog("--3");
    sleep(1);
    focuser_pos = focuserDevice->getNumber("FOCUS_ABSOLUTE_POSITION");
    IDLog("--4");
    sleep(1);
    if (focuser_pos==nullptr) IDLog("error getting focuser position");
    IDLog("--5");*/
    //postarget=posmin;
    //focuser_pos->np[0].value=postarget;
    //camera_client->sendNewNumber(focuser_pos);

    while(HICClientRunning){
            //IDLog("In Thread\n");
            sleep(1);
    };
    camera_client->disconnectServer();
            IDLog("Exit Thread\n");
    ( void ) arg;
    pthread_exit ( NULL );
}



/**************************************************************************************
**
***************************************************************************************/
MyClient::MyClient()
{
    cameraDevice = nullptr;
    focuserDevice = nullptr;
}



/**************************************************************************************
**
***************************************************************************************/
void MyClient::newDevice(INDI::BaseDevice *dp)
{
    if (strcmp(dp->getDeviceName(), clientCamera) == 0) {
        IDLog("Receiving %s Device...\n", dp->getDeviceName());
        cameraDevice = dp;
    }
    if (strcmp(dp->getDeviceName(), clientFocuser) == 0) {
        IDLog("Receiving %s Device...\n", dp->getDeviceName());
        focuserDevice = dp;
    }

}

/**************************************************************************************
**
*************************************************************************************/
void MyClient::newProperty(INDI::Property *property)
{
    if (strcmp(property->getDeviceName(), clientFocuser) == 0)
    {
        IDLog("Receiving %s %s property...\n",property->getDeviceName(), property->getName());
    }

    if (strcmp(property->getDeviceName(), clientCamera) == 0 && strcmp(property->getName(), "CONNECTION") == 0)
    {
        connectDevice(clientCamera);
        return;
    }
    if (strcmp(property->getDeviceName(), clientFocuser) == 0 && strcmp(property->getName(), "CONNECTION") == 0)
    {
        connectDevice(clientFocuser);
        return;
    }
    if (strcmp(property->getDeviceName(), clientFocuser) == 0 && strcmp(property->getName(), "ABS_FOCUS_POSITION") == 0)
    {
        INumberVectorProperty *focuser_pos = nullptr;
        focuser_pos = focuserDevice->getNumber("ABS_FOCUS_POSITION");
        focuser_pos->np[0].value = postarget;
        sendNewNumber(focuser_pos);
        return;
    }

}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newNumber(INumberVectorProperty *nvp)
{
    if (strcmp(nvp->device, clientFocuser) == 0 && strcmp(nvp->name, "ABS_FOCUS_POSITION") == 0)
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

    }
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
    if (strcmp(dp->getDeviceName(), MYCCD) != 0)
        return;

    IDLog("Recveing message from Server:%s\n", dp->messageQueue(messageID).c_str());
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newBLOB(IBLOB *bp)
{

    cameraImage.LoadFromBlob(bp);
    IDLog("POS=%i HFD=%f MED=%f\n", postarget, cameraImage.hfd,cameraImage.med);

    focuspoint.hfd = cameraImage.hfd;
    focuspoint.pos = postarget;
    focuscurve.push_back(focuspoint);
    posvector.push_back((double)focuspoint.pos);
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


