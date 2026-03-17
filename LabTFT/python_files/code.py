import lib
import time
import driver
import colors

'''
if __name__ == "__main__":
    print("Cridem Config_Pins")
    driver.Config_Pins()

    print("Cridem SPI_TFT_Reset")
    driver.SPI_TFT_Reset()

    while(1):
        a = 0
'''

if __name__ == "__main__":
    try:
        lib.init()

        # Pintar pantalla completa de negre i imprimir temps transcorregut
        print("Pintant pantalla completa...")
        start = time.time()
        lib.clear_screen(colors.GREEN)
        end = time.time()

        duration = end - start
        print(f"Ha trigat {duration:.6f} segons a pintar la pantalla completa.")

        time.sleep(5)

    finally:
        lib.quit()
