import RPi.GPIO as GPIO
import time

''' **************   ATENCIÓ  *********************
    **                                           **
    **  En aquest codi s'han triat uns pins: 17, **
    **  22, 23, 24 per la interfície i el mode   **
    **  BCM. Ajusteu i arregleu per les vostres  **
    **  connexions !!!!!!!                       **
    *********************************************** '''

def Config_GPIO_AD_SPI():
	#Set GPIO pins to interface the AD converter via SPI
	GPIO.setup(17,GPIO.IN) 
	#Data Input, internal pull-up, connected to DO in the AD-SPI
	GPIO.setup(22,GPIO.OUT) #Data Output, connected to DI in the AD-SPI 
	GPIO.setup(23,GPIO.OUT) #Clock Output, connected to SCLK in the AD-SPI
	GPIO.setup(24,GPIO.OUT) #Chip Select, connect to CS, need to use the AD-SPI

	GPIO.output(24,GPIO.HIGH) #AD-SPI Component not selected
	GPIO.output(23,GPIO.LOW)  #AD-SPI Clock initially low
	GPIO.output(22,GPIO.LOW) #AD-SPI DI initially low
	return

def Send_Bit_AD_SPI(val):
	if val==1:
		GPIO.output(22,GPIO.HIGH)
	if val==0:
		GPIO.output(22,GPIO.LOW) 
	GPIO.output(23,GPIO.LOW)  #Clock low
	GPIO.output(23,GPIO.HIGH) #Clock high
	return

def Chip_Select_AD_SPI(val):
	if val==0:
		GPIO.output(24,GPIO.LOW)  #AD-SPI CS
	if val==1:
		GPIO.output(24,GPIO.HIGH)  #AD-SPI deselect
	return

def Read_Bit_AD_SPI():
	GPIO.output(23,GPIO.LOW)  #Clock low
	GPIO.output(23,GPIO.HIGH) #Clock high
	return GPIO.input(17)

def Read_Channel_0_AD_SPI():
	Chip_Select_AD_SPI(0)	#Chip select
	Send_Bit_AD_SPI(1)	#Send Start
	Send_Bit_AD_SPI(1)	#Send Single-Ended Mode
	Send_Bit_AD_SPI(0)	#Send Channel 0 bit 0
	Send_Bit_AD_SPI(0)	#Send Channel 0 bit 1
	Send_Bit_AD_SPI(0)	#Send Channel 0 bit 2
	Send_Bit_AD_SPI(0)	#Send Dont Care bit
	Read_Bit_AD_SPI()	#Read a NULL bit
	value=0
	for i in range(12):
		value=2*value+Read_Bit_AD_SPI()	#Read a bit
	Chip_Select_AD_SPI(1)
	return value



GPIO.setmode(GPIO.BCM)  
GPIO.setwarnings(False) 
	
print ("Provant")

Config_GPIO_AD_SPI()

while True:
	print(f"Converted value from MCP3204: {Read_Channel_0_AD_SPI()}")
	time.sleep(0.1)

GPIO.cleanup


