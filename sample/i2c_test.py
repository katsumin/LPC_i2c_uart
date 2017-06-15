#!/usr/bin/env python
# -*- coding: utf-8 -*-

import smbus
import threading
import time
import sys
import datetime


# 初期化（38400bps）
# 0:300, 1:600, 2:1200, 3:2400, 4:4800, 5:9600, 6:19200, 7:38400, 8:57600, 9:115200
event = threading.Event()
lock = threading.Lock()
i2c = smbus.SMBus(1)
i2c.write_byte(0x40, 0x07)


# 受信データ数を返す
def receiveCount() :
  lock.acquire()
  try :
    count = i2c.read_byte(0x40)
  finally :
    lock.release()
  return count


# データ受信
def receiveData() :
  lock.acquire()
  try :
    data = i2c.read_byte(0x41)
  finally :
    lock.release()
  return data


# データ送信
def sendData(data) :
  lock.acquire()
  try :
    i2c.write_byte(0x41, data)
  finally :
    lock.release()


# 受信データ列を返す
def receiveMultiData(len) :
  multiData = bytearray()
  count = 0
  timeout = 10
  while event.is_set() != True :
    pre = count
    count = receiveCount()
    # 要求するデータ数を超えていたら、データ引き取り
    if count >= len :
      multiData = bytearray(count)
      for i in range(count) :
        multiData[i] = receiveData()
      return multiData
#    else :
#      print "count: {0}".format(count)

    if pre == count :
      time.sleep(0.1) # 0.1s wait
      timeout = timeout - 1
      if timeout < 1 :
        print 'receive timeout'
        return multiData
    else :
      # 受信があれば、タイムアウトカウンタをリセット
      timeout = 10
  # 終了イベントで抜ける
  print 'receive exit'
  return multiData


# データ列を送信する
def sendMultiData(multiData) :
  for i in range(multiData.__len__()) :
    sendData(ord(multiData[i]))


# 受信タスク
def rcvTask() :
  while event.is_set() != True :
    multiData = receiveMultiData(26)
    if multiData.__len__() < 26 :
      print "receive {0} less 26.".format(multiData.__len__())
      break
    print "{0} << ({1})".format(multiData,multiData.__len__())
  print 'receive exit'


# 送信タスク
def sndTask() :
  while event.is_set() != True :
    d = datetime.datetime.today()
    multiData = "{0:04d}/{1:02d}/{2:02d} {3:02d}:{4:02d}:{5:02d}.{6:06d}".format(d.year, d.month, d.day, d.hour, d.minute, d.second, d.microsecond)
    sendMultiData(multiData)
    time.sleep(0.1) # wait
  print 'send exit'


# キー待ちタスク
def keyTask() :
  code = raw_input("")
  event.set()


# 受信バッファ・クリア
for i in range(receiveCount()) :
  receiveData()


thread1 = threading.Thread(target=rcvTask)
thread2 = threading.Thread(target=sndTask)
thread3 = threading.Thread(target=keyTask)
thread1.start()
thread2.start()
thread3.start()
thread1.join()
thread2.join()
thread3.join()

