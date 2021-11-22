# Import the libraries we need

import requests
import time
from megapi import *

if __name__ == '__main__':
	#bot = MegaPi()
	#bot.start('/dev/ttyUSB0')
    #bot.motorRun(1,0)


#Let us fetch data and evaluate label.

    while True:
        r = requests.get('http://ur-pi.local:3333/')
        label = r.text
        if label == "no":
            #bot.motorRun(1,10)
            time.sleep(2)
            print("no")
            #bot.motorRun(1.0)
        elif label == "cone":
            #bot.motorRun(2,10)
            #bot.motorRun(1,5)
            print("cone - turn")
            time.sleep(3)
            #bot.motorRun(2,-10)
            print("cone - turn back")
            time.sleep(3)
        elif label == "LEGOMan":
            #bot.motorRun(1,30)
            print("kill the lego man")
            time.sleep(1)