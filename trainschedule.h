#ifndef TRAIN_SCHEDULE_H
#define TRAIN_SCHEDULE_H

#include "common.h"

#define TRAIN_RUNNING       1
#define TRAIN_RESCHEDULED   2
#define TRAIN_CANCELLED     3
#define TRAIN_SPECIAL       4


extern TrainSchedule *scheduleHead;

void initializeTrainScheduleModule(void);

void addTrainSchedule(void);

void viewTrainSchedules(void);

void searchTrainSchedule(void);

void updateTrainSchedule(void);

void deleteTrainSchedule(void);

int saveTrainScheduleData(void);

void loadTrainScheduleData(void);
TrainSchedule *findTrainScheduleByDate(int trainNumber, const char *journeyDate);

#endif
