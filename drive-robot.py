# Import the libraries we need

import requests
import time
from megapi import *

def onForwardFinish(slot):
    sleep(0.4);
    return;




if __name__ == '__main__':
    bot = MegaPi()
    bot.start('/dev/ttyUSB0')
    bot.encoderMotorRun(1,0)
    bot.encoderMotorRun(2,0)
 
#Let us fetch data and evaluate label.

while True:
    r = requests.get('http://ur-pi.local:3333/')
    label = r.text
    if label == "no":
	print("no")
        sleep(0.4);
	bot.encoderMotorMove(1, 500, 2000, onForwardFinish);
        time.sleep(2);
    elif label == "cone":
        print("cone - turn")
	sleep(0.4);
	bot.encoderMotorMove(2, 500, 500, onForwardFinish);
        bot.encoderMotorMove(1, 500, 2000, onForwardFinish);
        time.sleep(3);
        bot.encoderMotorMove(2, 500, -500, onForwardFinish);
        print("cone - turn back")
        time.sleep(3);
    
    #elif label == "LEGOMan":
#	sleep(0.4);
#        bot.encoderMotorRun(1, 50, 100, 0);
#        print("kill the lego man")
#        time.sleep(1);
