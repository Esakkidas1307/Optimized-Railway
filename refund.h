#ifndef REFUND_H
#define REFUND_H
#include "reserved_booking.h"
float calculateRefundAmount(float farePaid);
int confirmRefundAmount(float farePaid, float refundAmount);
#endif
