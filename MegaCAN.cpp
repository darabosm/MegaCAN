#include "MegaCAN.h"

#include <Arduino.h>

// Old implementation that allows for global baseID variable. This is for legacy reasons, do not use this in new code.
MegaCAN::MegaCAN() : _baseId(baseID) {}

MegaCAN::MegaCAN(const uint32_t baseId) : _baseId(baseId) {}

void MegaCAN::processMSreq(uint32_t msgCore, const uint8_t msgData[3], MegaCAN_message_t &msg) {
  // Process message core
  msg.core.toTable = (msgCore & ~(0b11111111111111111111111111111011)) << 2;   // set bit 4 of toTable to bit 2 of msgCore
  msg.core.toTable |= (msgCore & ~(0b11111111111111111111111110000111)) >> 3;  // set bits 3-0 of toTable to bits 6-3 of msgCore
  msg.core.toID = (msgCore & ~(0b11111111111111111111100001111111)) >> 7;      // set ToID to bits 10-7 of msgCore
  msg.core.fromID = (msgCore & ~(0b11111111111111111000011111111111)) >> 11;   // set FromID to bits 14-11 of msgCore
  msg.core.msgType = (msgCore & ~(0b11111111111111000111111111111111)) >> 15;  // set msgType to bits 17-15 of msgCore
  msg.core.toOffset = (msgCore & ~(0b11100000000000111111111111111111)) >> 18; // set toOffset to bits 28-18 of msgCore

  // Process message data
  msg.data.request.varBlk = msgData[0];
  msg.data.request.varOffset = msgData[1];
  msg.data.request.varOffset = (msg.data.request.varOffset << 3) | (msgData[2] >> 5);
  msg.data.request.varByt = msgData[2] & ~(0b11110000);
}

void MegaCAN::setMSresp(MegaCAN_message_t recMsg, MegaCAN_message_t &respMsg, uint16_t var0, uint16_t var1, uint16_t var2, uint16_t var3) {
  respMsg.responseCore = 0;
  respMsg.responseCore |= (recMsg.data.request.varBlk >> 4) << 2;            // isolate table bit 4, write to core bit 2
  respMsg.responseCore |= (recMsg.data.request.varBlk & ~(0b00010000)) << 3; // isolate table bits 3-0, write to core bits 6-3
  respMsg.responseCore |= recMsg.core.fromID << 7;                           // write ToID bits 3-0 to core bits 10-7
  respMsg.responseCore |= recMsg.core.toID << 11;                            // write FromID bits 3-0 to core bits 14-11
  respMsg.responseCore |= 2 << 15;                                           // write MsgType bits 2-0 to core bits 17-15
  respMsg.responseCore |= recMsg.data.request.varOffset << 18;               // write Offset bits 10-0 to core bits 28-18

  uint16_t var[4] = { 0 };
  for (int i = 0; i < (recMsg.data.request.varByt / 2); i++) {
    switch (i) {
      case 0:
        var[0] = var0;
        break;
      case 1:
        var[1] = var1;
        break;
      case 2:
        var[2] = var2;
        break;
      case 3:
        var[3] = var3;
        break;
    }
    respMsg.data.response[2*i] = highByte(var[i]);
    respMsg.data.response[2*i+1] = lowByte(var[i]);
  }
}

void MegaCAN::getBCastData(uint32_t id, const uint8_t data[8], MegaCAN_broadcast_message_t &msg) {
  switch (id - _baseId) {
    case 0:
      msg.seconds = (data[0] << 8) | data[1];
      msg.pw1 = ((data[2] << 8) | data[3]) / (float)1000;
      msg.pw2 = ((data[4] << 8) | data[5]) / (float)1000;
      msg.rpm = (data[6] << 8) | data[7];
      break;
    case 1:
      msg.adv_deg = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.squirt = data[2];
      msg.engine = data[3];
      msg.afrtgt1 = data[4] / (float)10;
      msg.afrtgt2 = data[5] / (float)10;
      msg.wbo2_en1 = data[6];
      msg.wbo2_en2 = data[7];
      break;
    case 2:
      msg.baro = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.map = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.mat = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.clt = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.mat = (msg.mat - 32) * 5 / 9;
          msg.clt = (msg.clt - 32) * 5 / 9;
      #endif
      break;
    case 3:
      msg.tps = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.batt = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.afr1_old = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.afr2_old = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      break;
    case 4:
      msg.knock = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.egocor1 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.egocor2 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.aircor = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 5:
      msg.warmcor = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.tpsaccel = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.tpsfuelcut = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.barocor = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 6:
      msg.totalcor = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.ve1 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.ve2 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.iacstep = static_cast<int16_t>(data[6] << 8) | data[7];
      msg.iacduty = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)392 / (float)1000;
      break;
    case 7:
      msg.cold_adv_deg = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.TPSdot = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.MAPdot = static_cast<int16_t>(data[4] << 8) | data[5];
      msg.RPMdot = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)10;
      break;
    case 8:
      msg.MAFload = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.fuelload = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.fuelcor = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.MAF = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)100;
      break;
    case 9:
      msg.egoV1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)100;
      msg.egoV2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)100;
      msg.dwell = ((data[4] << 8) | data[5]) / (float)10;
      msg.dwell_trl = ((data[6] << 8) | data[7]) / (float)10;
      break;
    case 10:
      msg.status1 = data[0];
      msg.status2 = data[1];
      msg.status3 = data[2];
      msg.status4 = data[3];
      msg.status5 = static_cast<int16_t>((data[4] << 8) | data[5]);
      msg.status6 = data[6];
      msg.status7 = data[7];
      break;
    case 11:
      msg.fuelload2 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.ignload = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.ignload2 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.airtemp = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.airtemp = (msg.airtemp - 32) * 5 / 9;
      #endif
    break;
    case 12:
      msg.wallfuel1 = static_cast<int32_t>((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]) / (float)100;
      msg.wallfuel2 = static_cast<int32_t>((data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]) / (float)100;
      break;
    case 13:
      msg.sensors1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.sensors2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.sensors3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.sensors4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 14:
      msg.sensors5 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.sensors6 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.sensors7 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.sensors8 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 15:
      msg.sensors9 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.sensors10 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.sensors11 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.sensors12 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 16:
      msg.sensors13 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.sensors14 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.sensors15 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.sensors16 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 17:
      msg.boost_targ_1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.boost_targ_2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.boostduty = data[4];
      msg.boostduty2 = data[5];
      msg.maf_volts = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 18:
      msg.pwseq1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)1000;
      msg.pwseq2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)1000;
      msg.pwseq3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.pwseq4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 19:
      msg.pwseq5 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)1000;
      msg.pwseq6 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)1000;
      msg.pwseq7 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.pwseq8 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 20:
      msg.pwseq9 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)1000;
      msg.pwseq10 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)1000;
      msg.pwseq11 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.pwseq12 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 21:
      msg.pwseq13 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)1000;
      msg.pwseq14 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)1000;
      msg.pwseq15 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.pwseq16 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 22:
      msg.egt1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.egt2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.egt3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.egt4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.egt1 = (msg.egt1 - 32) * 5 / 9;
          msg.egt2 = (msg.egt2 - 32) * 5 / 9;
          msg.egt3 = (msg.egt3 - 32) * 5 / 9;
          msg.egt4 = (msg.egt4 - 32) * 5 / 9;
      #endif
      break;
    case 23:
      msg.egt5 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.egt6 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.egt7 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.egt8 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.egt5 = (msg.egt5 - 32) * 5 / 9;
          msg.egt6 = (msg.egt6 - 32) * 5 / 9;
          msg.egt7 = (msg.egt7 - 32) * 5 / 9;
          msg.egt8 = (msg.egt8 - 32) * 5 / 9;
      #endif
      break;
    case 24:
      msg.egt9 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.egt10 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.egt11 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.egt12 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.egt9 = (msg.egt9 - 32) * 5 / 9;
          msg.egt10 = (msg.egt10 - 32) * 5 / 9;
          msg.egt11 = (msg.egt11 - 32) * 5 / 9;
          msg.egt12 = (msg.egt12 - 32) * 5 / 9;
      #endif
      break;
    case 25:
      msg.egt13 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.egt14 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.egt15 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.egt16 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.egt13 = (msg.egt13 - 32) * 5 / 9;
          msg.egt14 = (msg.egt14 - 32) * 5 / 9;
          msg.egt15 = (msg.egt15 - 32) * 5 / 9;
          msg.egt16 = (msg.egt16 - 32) * 5 / 9;
      #endif
      break;
    case 26:
      msg.nitrous1_duty = data[0];
      msg.nitrous2_duty = data[1];
      msg.nitrous_timer_out = ((data[2] << 8) | data[3]) / (float)1000;
      msg.n2o_addfuel = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.n2o_retard = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 27:
      msg.canpwmin1 = static_cast<int16_t>(data[0] << 8) | data[1];
      msg.canpwmin2 = static_cast<int16_t>(data[2] << 8) | data[3];
      msg.canpwmin3 = static_cast<int16_t>(data[4] << 8) | data[5];
      msg.canpwmin4 = static_cast<int16_t>(data[6] << 8) | data[7];
      break;
    case 28:
      msg.cl_idle_targ_rpm = (data[0] << 8) | data[1];
      msg.tpsadc = static_cast<int16_t>(data[2] << 8) | data[3];
      msg.eaeload = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.afrload = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 29:
      msg.EAEfcor1 = ((data[0] << 8) | data[1]) / (float)10;
      msg.EAEfcor2 = ((data[2] << 8) | data[3]) / (float)10;
      msg.VSS1dot = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.VSS2dot = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 30:
      msg.accelx = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)1000;
      msg.accely = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)1000;
      msg.accelz = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)1000;
      msg.stream_level = data[6];
      msg.water_duty = data[7];
      break;
    case 31:
      msg.AFR1 = data[0] / (float)10;
      msg.AFR2 = data[1] / (float)10;
      msg.AFR3 = data[2] / (float)10;
      msg.AFR4 = data[3] / (float)10;
      msg.AFR5 = data[4] / (float)10;
      msg.AFR6 = data[5] / (float)10;
      msg.AFR7 = data[6] / (float)10;
      msg.AFR8 = data[7] / (float)10;
      break;
    case 32:
      msg.AFR9 = data[0] / (float)10;
      msg.AFR10 = data[1] / (float)10;
      msg.AFR11 = data[2] / (float)10;
      msg.AFR12 = data[3] / (float)10;
      msg.AFR13 = data[4] / (float)10;
      msg.AFR14 = data[5] / (float)10;
      msg.AFR15 = data[6] / (float)10;
      msg.AFR16 = data[7] / (float)10;
      break;
    case 33:
      msg.duty_pwm_1 = data[0];
      msg.duty_pwm_2 = data[1];
      msg.duty_pwm_3 = data[2];
      msg.duty_pwm_4 = data[3];
      msg.duty_pwm_5 = data[4];
      msg.duty_pwm_6 = data[5];
      msg.gear = data[6];
      msg.status8 = data[7];
      break;
    case 34:
      msg.EGOv1 = static_cast<int16_t>((data[0] << 8) | data[1]) * (float)489 / (float)100000;
      msg.EGOv2 = static_cast<int16_t>((data[2] << 8) | data[3]) * (float)489 / (float)100000;
      msg.EGOv3 = static_cast<int16_t>((data[4] << 8) | data[5]) * (float)489 / (float)100000;
      msg.EGOv4 = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)489 / (float)100000;
      break;
    case 35:
      msg.EGOv5 = static_cast<int16_t>((data[0] << 8) | data[1]) * (float)489 / (float)100000;
      msg.EGOv6 = static_cast<int16_t>((data[2] << 8) | data[3]) * (float)489 / (float)100000;
      msg.EGOv7 = static_cast<int16_t>((data[4] << 8) | data[5]) * (float)489 / (float)100000;
      msg.EGOv8 = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)489 / (float)100000;
      break;
    case 36:
      msg.EGOv9 = static_cast<int16_t>((data[0] << 8) | data[1]) * (float)489 / (float)100000;
      msg.EGOv10 = static_cast<int16_t>((data[2] << 8) | data[3]) * (float)489 / (float)100000;
      msg.EGOv11 = static_cast<int16_t>((data[4] << 8) | data[5]) * (float)489 / (float)100000;
      msg.EGOv12 = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)489 / (float)100000;
      break;
    case 37:
      msg.EGOv13 = static_cast<int16_t>((data[0] << 8) | data[1]) * (float)489 / (float)100000;
      msg.EGOv14 = static_cast<int16_t>((data[2] << 8) | data[3]) * (float)489 / (float)100000;
      msg.EGOv15 = static_cast<int16_t>((data[4] << 8) | data[5]) * (float)489 / (float)100000;
      msg.EGOv16 = static_cast<int16_t>((data[6] << 8) | data[7]) * (float)489 / (float)100000;
      break;
    case 38:
      msg.EGOcor1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.EGOcor2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.EGOcor3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.EGOcor4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 39:
      msg.EGOcor5 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.EGOcor6 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.EGOcor7 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.EGOcor8 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 40:
      msg.EGOcor9 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.EGOcor10 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.EGOcor11 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.EGOcor12 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 41:
      msg.EGOcor13 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.EGOcor14 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.EGOcor15 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.EGOcor16 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 42:
      msg.VSS1 = ((data[0] << 8) | data[1]) / (float)10;
      msg.VSS2 = ((data[2] << 8) | data[3]) / (float)10;
      msg.VSS3 = ((data[4] << 8) | data[5]) / (float)10;
      msg.VSS4 = ((data[6] << 8) | data[7]) / (float)10;
      break;
    case 43:
      msg.synccnt = data[0];
      msg.syncreason = data[1];
      msg.sd_filenum = (data[2] << 8) | data[3];
      msg.sd_error = data[4];
      msg.sd_phase = data[5];
      msg.sd_status = data[6];
      msg.timing_err = data[7];
      break;
    case 44:
      msg.vvt_ang1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.vvt_ang2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.vvt_ang3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.vvt_ang4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 45:
      msg.vvt_target1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.vvt_target2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.vvt_target3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.vvt_target4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 46:
      msg.vvt_duty1 = data[0] * (float)392 / (float)1000;
      msg.vvt_duty1 = data[1] * (float)392 / (float)1000;
      msg.vvt_duty1 = data[2] * (float)392 / (float)1000;
      msg.vvt_duty1 = data[3] * (float)392 / (float)1000;
      msg.inj_timing_pri = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.inj_timing_sec = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 47:
      msg.fuel_pct = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.tps_accel = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.SS1 = ((data[4] << 8) | data[5]) * 10;
      msg.SS2 = ((data[6] << 8) | data[7]) * 10;
      break;
    case 48:
      msg.knock_cyl1 = data[0] * (float)4 / (float)10;
      msg.knock_cyl2 = data[1] * (float)4 / (float)10;
      msg.knock_cyl3 = data[2] * (float)4 / (float)10;
      msg.knock_cyl4 = data[3] * (float)4 / (float)10;
      msg.knock_cyl5 = data[4] * (float)4 / (float)10;
      msg.knock_cyl6 = data[5] * (float)4 / (float)10;
      msg.knock_cyl7 = data[6] * (float)4 / (float)10;
      msg.knock_cyl8 = data[7] * (float)4 / (float)10;
      break;
    case 49:
      msg.knock_cyl9 = data[0] * (float)4 / (float)10;
      msg.knock_cyl10 = data[1] * (float)4 / (float)10;
      msg.knock_cyl11 = data[2] * (float)4 / (float)10;
      msg.knock_cyl12 = data[3] * (float)4 / (float)10;
      msg.knock_cyl13 = data[4] * (float)4 / (float)10;
      msg.knock_cyl14 = data[5] * (float)4 / (float)10;
      msg.knock_cyl15 = data[6] * (float)4 / (float)10;
      msg.knock_cyl16 = data[7] * (float)4 / (float)10;
      break;
    case 50:
      msg.map_accel = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.total_accel = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.launch_timer = ((data[4] << 8) | data[5]) / (float)1000;
      msg.launch_retard = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 51:
      msg.porta = data[0];
      msg.portb = data[1];
      msg.porteh = data[2];
      msg.portk = data[3];
      msg.portmj = data[4];
      msg.portp = data[5];
      msg.portt = data[6];
      msg.cel_errorcode = data[7];
      break;
    case 52:
      msg.canin1 = data[0];
      msg.canin2 = data[1];
      msg.canout = data[2];
      msg.knk_rtd = data[3] / (float)10;
      msg.fuelflow = (data[4] << 8) | data[5];
      msg.fuelcons = (data[6] << 8) | data[7];
      break;
    case 53:
      msg.fuel_press1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.fuel_press2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.fuel_temp1 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.fuel_temp2 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      #ifdef CELSIUS
          msg.fuel_temp1 = (msg.fuel_temp1 - 32) * 5 / 9;
          msg.fuel_temp2 = (msg.fuel_temp2 - 32) * 5 / 9;
      #endif
      break;
    case 54:
      msg.batt_cur = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.cel_status = (data[2] << 8) | data[3];
      msg.fp_duty = data[4] * (float)392 / (float)1000;
      msg.alt_duty = data[5];
      msg.load_duty = data[6];
      msg.alt_targv = data[7] / (float)10;
      break;
    case 55:
      msg.looptime = (data[0] << 8) | data[1];
      msg.fueltemp_cor = ((data[2] << 8) | data[3]) / (float)10;
      msg.fuelpress_cor = ((data[4] << 8) | data[5]) / (float)10;
      msg.ltt_cor = data[6] / (float)10;
      msg.sp1 = data[7];
      break;
    case 56:
      msg.tc_retard = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.cel_retard = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.fc_retard = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.als_addfuel = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 57:
      msg.base_advance = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.idle_cor_advance = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.mat_retard = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.flex_advance = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 58:
      msg.adv1 = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.adv2 = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.adv3 = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.adv4 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)10;
      break;
    case 59:
      msg.revlim_retard = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.als_timing = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.ext_advance = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.deadtime1 = static_cast<int16_t>((data[6] << 8) | data[7]) / (float)1000;
      break;
    case 60:
      msg.launch_timing = static_cast<int16_t>((data[0] << 8) | data[1]) / (float)10;
      msg.step3_timing = static_cast<int16_t>((data[2] << 8) | data[3]) / (float)10;
      msg.vsslaunch_retard = static_cast<int16_t>((data[4] << 8) | data[5]) / (float)10;
      msg.cel_status2 = (data[6] << 8) | data[7];
      break;
    case 61:
      msg.gps_latdeg = data[0];
      msg.gps_latmin = data[1];
      msg.gps_latmmin = (data[2] << 8) | data[3];
      msg.gps_londeg = data[4];
      msg.gps_lonmin = data[5];
      msg.gps_lonmmin = (data[6] << 8) | data[7];
      break;
    case 62:
      msg.gps_outstatus = data[0];
      msg.gps_altk = data[1];
      msg.gps_altm = (data[2] << 8) | data[3];
      msg.gps_speed = ((data[4] << 8) | data[5]) / (float)10;
      msg.gps_course = ((data[6] << 8) | data[7]) / (float)10;
      break;
    case 63:
      msg.generic_pid_duty1 = data[0] * (float)392 / (float)1000;
      msg.generic_pid_duty2 = data[1] * (float)392 / (float)1000;
      break;
  }
}
