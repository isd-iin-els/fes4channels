#include <iostream>
#include <time.h>
#include "esp_system.h"
#include "rom/ets_sys.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "SistemasdeControle/headers/primitiveLibs/LinAlg/matrix.h"
#include "SistemasdeControle/embeddedTools/signalAnalysis/systemLoop.h"
#include "SistemasdeControle/embeddedTools/communicationLibs/wifi/wifista.h"
#include "I2Cdev.h"
// #include "MPU6050.h"
#include "SistemasdeControle/embeddedTools/sensors/adxl345.h"
#include "SistemasdeControle/embeddedTools/sensors/kalmanfilter.h"
#include "SistemasdeControle/embeddedTools/controlLibs/pid.h"

uint8_t modPin[4] = {27,4,12,5},
        levelPin[4]   = {13,19,26,18};
Devices::fes4channels dispositivo(levelPin,modPin,4);
ControlHandler::PID<long double> pid[2];
esp_timer_create_args_t periodic_timer_args;

bool flag = false,controle = false;
Communication::WifiSTA* wifi = Communication::WifiSTA::GetInstance(); uint32_t wificounter = 0; 
std::stringstream wifidata;

// MPU6050 mpu1, mpu2(0x69);
adxl345 acc;
int16_t ax1,ay1,az1,gx1,gy1,gz1;
int16_t ax2,ay2,az2,gx2,gy2,gz2, counter = 0;
double pitch1 = 0, roll1 = 0, fpitch1, froll1;
double pitch2 = 0, roll2 = 0, fpitch2, froll2;
double ref1, ref2;
KALMAN pfilter1(0.016), rfilter1(0.016), pfilter2(0.016), rfilter2(0.016);
esp_timer_handle_t periodic_timer;

int sign(double value) {  if (value > 0) return 1;  else return -1; }
static void mpuRead(void *para);

#define GYRO_FULLSCALE			 (250)
#if   GYRO_FULLSCALE == 250
    #define GyroAxis_Sensitive (float)(131.0)
#endif

static void wifiCallback(void){ 
    if (!flag)
    {
        flag = true;
        periodic_timer_args.callback = &mpuRead;
        periodic_timer_args.name = "ccdfkgdjf";
        ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 100000));
    }  

    LinAlg::Matrix<double> code = wifi->getData();
	std::cout << code;
    if(code(0,0) == 0){
        //  for(uint8_t i = 1; i < 5; ++i)
            dispositivo.fes[0].setPowerLevel(code(0,1)); 
            dispositivo.fes[1].setPowerLevel(code(0,2)); 
    } else if(code(0,0) == 1){
        ref1 = code(0,1);
        ref2 = code(0,2);
    } else if(code(0,0) == 2)
    {
        pid[0].setParams(code(0,3),code(0,4),code(0,5));  pid[0].setLimits(code(0,2),code(0,1));
        pid[1].setParams(code(0,8),code(0,9),code(0,10)); pid[1].setLimits(code(0,7),code(0,6));
        controle = !controle;
    }
   
    //wifi1 << str.str().c_str();
}

void mpuRead(void *para){
   
    // mpu1.getMotion6(&ax1, &ay1, &az1, &gx1, &gy1, &gz1);
    // ax1 = fabs(ax1); ay1 = fabs(ay1); az1 = fabs(az1);
    // pitch1 += 0.1 * (atan2(double(-ax1), sqrt(double(az1 * az1) + double(ay1 * ay1))) * 57.2958 - pitch1);
    // roll1 +=  0.1 * ( atan2(double(-ay1), sqrt(double(az1 * az1) + double(ax1 * ax1))) * 57.2958 - roll1);
    // fpitch1 = pfilter1.filter(pitch1, gx1/GyroAxis_Sensitive);
    // froll1 = rfilter1.filter(roll1, gy1/GyroAxis_Sensitive);

    // mpu2.getMotion6(&ax2, &ay2, &az2, &gx2, &gy2, &gz2);
    // ax2 = fabs(ax2); ay2 = fabs(ay2); az2 = fabs(az2);
    // pitch2 += 0.1 * (atan2(double(-ax2), sqrt(double((az2) * (az2)) + double((ay2) * (ay2)))) * 57.2958 - pitch2);
    // roll2 += 0.1 * (atan2(double(-ay2), sqrt(double((az2) * (az2)) + double((ax2) * (ax2)))) * 57.2958 - roll2);
    // fpitch2 = pfilter2.filter(pitch2, gx2/GyroAxis_Sensitive);
    // froll2 = rfilter2.filter(roll2, gy2/GyroAxis_Sensitive);
    // std::cout << ax1 << " " << ay1 <<" " << az1 << " " << gx1 <<" " << gy1 << " " << gz1 << " " << ax2 << " " << ay2 <<" " << az2 << " " << gx2 <<" " << gy2 << " " << gz2;
    acc.read();
    // if(counter++ == 50){
        // counter = 0;
    std::cout << acc.get_pitch() << " " << acc.get_roll() << "\t";
//R = Referencia E = Estensão F = Felxão AA = Abdução Adução
    // Serial.print("REF EF RAA AA u1 u2:\t"); Serial.print(ref1); Serial.print("\t"); Serial.print(pitch1-pitch2); Serial.print("\t"); Serial.print(ref2); Serial.print("\t"); Serial.print(roll1-roll2); Serial.print("\t");
    // Serial.print("REF EF RAA AA u1 u2:\t"); Serial.print(ref1); Serial.print("\t"); Serial.print(pitch2); Serial.print("\t"); Serial.print(ref2); Serial.print("\t"); Serial.print(roll2); Serial.print("\t");

//OBS: tomar cuidado com o valor de saturação do pid, 
//     e somar o valor minimo do limiar motor 
    if(controle){
        double u;
        // double u = pid[0].OutputControl(ref1,pitch1-pitch2);
        // double u = pid[0].OutputControl(ref1, acc.get_pitch());
        // if (u > 0)
            // dispositivo.fes[0].setPowerLevel(u); 
        // else
        //     dispositivo.fes[1].setPowerLevel(-u); 
        // if(counter == 0)
        // std::cout << u << "\t"<< std::endl;
        wifidata << wificounter << " , " << acc.get_pitch()<< " , " << u << " , ";
        // u = pid[1].OutputControl(ref2,roll1-roll2);
        u = pid[1].OutputControl(ref2,acc.get_roll());
        // if (u > 0)
            dispositivo.fes[1].setPowerLevel(u); 
        // else
        //     dispositivo.fes[3].setPowerLevel(-u);
        // if(counter == 0){
        std::cout << u << std::endl;
        wifidata << acc.get_roll() << " , " << u;
        if((wificounter++)%5 == 0){
            wifi->write(wifidata.str().c_str());
            std::cout << wifidata.str().c_str();
            wifidata.str("");
        }else  std::cout <<  std::endl;
    }
}



void setup(){ 
    Serial.begin(115200);
    dispositivo.startLoop();  

    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
    #endif

    Serial.begin(115200);

    // initialize device
    // Serial.println("Initializing I2C devices...");
    // mpu1.initialize();
    // mpu2.initialize();

    // verify connection
    // Serial.println("Testing device connections...");
    // Serial.println(mpu1.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
    // Serial.println("Testing device connections...");
    // Serial.println(mpu2.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
    // mpu1.setXAccelOffset(-1929); mpu1.setYAccelOffset(621); mpu1.setZAccelOffset(5111);
    // mpu1.setXGyroOffset (-69); mpu1.setYGyroOffset (84); mpu1.setZGyroOffset (58);
    
    // mpu2.setXAccelOffset(-2219); mpu2.setYAccelOffset(-2265); mpu2.setZAccelOffset(4749);
    // mpu2.setXGyroOffset (20); mpu2.setYGyroOffset (28); mpu2.setZGyroOffset (-24);
    // mpu1.CalibrateAccel(6);mpu1.CalibrateGyro(6);mpu1.PrintActiveOffsets();
    // mpu2.CalibrateAccel(6); mpu2.CalibrateGyro(6);mpu2.PrintActiveOffsets();
    acc.init();
    pid[0].setParams(1,0.1,0); pid[0].setSampleTime(1); pid[0].setLimits(1.1,1.5);
    pid[1].setParams(1,0.1,0); pid[1].setSampleTime(1); pid[1].setLimits(1.1,1.8);
    
    std::cout << "Entrou1" << std::endl;
    wifi->connect(wifiCallback);
    std::cout << "Entrou6" << std::endl;
    wifi->initializeComunication(); 
  }

void loop(){
    
}