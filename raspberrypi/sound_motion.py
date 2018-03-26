import pygame
import ConfigParser
import RPi.GPIO as GPIO
import time
import os

def rileva_ascoltatore():
  if sensore_pir_attivo:
    return (GPIO.input(PIR_SENSOR_PIN) == GPIO.HIGH) 
  else:
    return False

base_path = '/mnt/samba/sound-motion/'

#Imposta la numerazione dei PIN GPIO
GPIO.setmode(GPIO.BCM)
#set PIR sensr pin as an input
PIR_SENSOR_PIN = 4
GPIO.setup(PIR_SENSOR_PIN, GPIO.IN)

#Leggi il file fi configurazione
config = ConfigParser.RawConfigParser()
config.read( os.path.join(base_path, 'sound_motion.cfg'))

#Recupera parametri da file configurazione
file_da_riprodurre = os.path.join(base_path, config.get("Generale","file_da_riprodurre"))
sensore_pir_attivo = config.getboolean("Parametri Sensori","sensore_pir_attivo")
tempo_massimo_senza_movimenti_sec = config.getint("Parametri Sensori","tempo_massimo_senza_movimenti_sec")
tempo_pausa_dopo_riproduzione_sec = config.getint("Parametri Sensori","tempo_pausa_dopo_riproduzione_sec")

#Variabili utili
ora = time.time()
istante_ultima_riproduzione = ora
musica_in_riproduzione = False
ascoltatore_rilevato = False
in_pausa_dopo_riproduzione = False
istante_ultimo_movimento = ora
ultimo_istante_riproduzione = 0

pygame.init()
pygame.mixer.init()

#while pygame.mixer.music.get_busy(): 
#  pygame.time.Clock().tick(10)

while True:
  
  ora = time.time()
  musica_in_riproduzione = pygame.mixer.music.get_busy()
  ascoltatore_rilevato = rileva_ascoltatore()

  if musica_in_riproduzione:
    print "Musica in riproduzione"
    ultimo_istante_riproduzione = ora
    if ascoltatore_rilevato:
      print "Movimento rilevato... reset timeout "
      istante_ultimo_movimento = ora
    
    print "Tempo trascorso senza movimento: " + str(ora - istante_ultimo_movimento )
    if ora - istante_ultimo_movimento > tempo_massimo_senza_movimenti_sec :
      print "Fermo musica"
      ultimo_istante_riproduzione = ora - tempo_pausa_dopo_riproduzione_sec -1
      pygame.mixer.music.fadeout(1000)
  else:
    print "No musica"
    if ascoltatore_rilevato :
      print "Tempo da ultima riproduzione: " + str(ora - ultimo_istante_riproduzione )
      if ora - ultimo_istante_riproduzione  > tempo_pausa_dopo_riproduzione_sec :
        print "Avvio la musica!"
        pygame.mixer.music.load(file_da_riprodurre)
        pygame.mixer.music.play()
      else:
        istante_ultimo_movimento = ora
  time.sleep( 1.5 )
  