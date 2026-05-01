import lib
import time
import colors

if __name__ == "__main__":
    try:
        lib.init()

        # Pintar pantalla completa de verd i imprimir temps transcorregut
        print("Pintant pantalla completa...")
        start = time.time()
        lib.clear_screen(colors.GREEN)
        end = time.time()

        duration = end - start
        print(f"Ha trigat {duration:.6f} segons a pintar la pantalla completa.")

        time.sleep(5)

    finally:
        lib.quit()
