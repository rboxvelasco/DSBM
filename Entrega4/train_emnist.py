#!/usr/bin/env python3
"""
train_emnist.py  –  Entrena una CNN lleugera sobre EMNIST Letters i exporta
                    un model .tflite quantitzat per a Raspberry Pi 2B.

Executa aquest script al PC (NO a la Raspberry):

  pip install tensorflow tensorflow-datasets
  python3 train_emnist.py

Genera: emnist_letters.tflite  (~200 KB)
Copia'l a la Raspberry:
  scp emnist_letters.tflite pi@<IP_RASPBERRY>:~/
"""

import os
import numpy as np
import tensorflow as tf
import tensorflow_datasets as tfds

# ── Configuració ───────────────────────────────────────────────────────────────
IMG_SIZE    = 28          # EMNIST és 28×28
NUM_CLASSES = 26          # A–Z
EPOCHS      = 15
BATCH_SIZE  = 256
MODEL_PATH  = "emnist_letters.tflite"

# Mapa de labels: índex 0 → 'A', 1 → 'B', ... 25 → 'Z'
# EMNIST Letters usa labels 1–26; en convertim a 0–25 al preprocess.
LABEL_MAP = [chr(ord('A') + i) for i in range(26)]

print(f"TensorFlow {tf.__version__}")
print(f"GPU disponible: {bool(tf.config.list_physical_devices('GPU'))}")


# ── Càrrega i preprocessat ────────────────────────────────────────────────────

def fix_orientation(img):
    """
    Les imatges EMNIST estan girades 90° i reflectides respecte a com
    les escriuria un humà. Aquesta correcció les deixa en orientació normal.
    """
    img = tf.transpose(img, perm=[1, 0, 2])   # transposició
    img = img[:, ::-1, :]                      # reflexió horitzontal
    return img


def preprocess(img, label):
    img = tf.cast(img, tf.float32) / 255.0    # normalitza a [0, 1]
    img = fix_orientation(img)
    label = label - 1                          # 1–26  →  0–25
    return img, label


def load_dataset():
    print("\nDescarregant EMNIST Letters (la primera vegada pot tardar ~2 min)...")
    (ds_train_raw, ds_test_raw), info = tfds.load(
        'emnist/letters',
        split=['train', 'test'],
        with_info=True,
        as_supervised=True,
    )
    n_train = info.splits['train'].num_examples
    n_test  = info.splits['test'].num_examples
    print(f"  Train: {n_train:,} mostres  |  Test: {n_test:,} mostres")

    ds_train = (ds_train_raw
                .map(preprocess, num_parallel_calls=tf.data.AUTOTUNE)
                .cache()
                .shuffle(10_000)
                .batch(BATCH_SIZE)
                .prefetch(tf.data.AUTOTUNE))

    ds_test = (ds_test_raw
               .map(preprocess, num_parallel_calls=tf.data.AUTOTUNE)
               .batch(BATCH_SIZE)
               .cache()
               .prefetch(tf.data.AUTOTUNE))

    return ds_train, ds_test, ds_train_raw


# ── Arquitectura ──────────────────────────────────────────────────────────────

def build_model():
    """
    CNN lleugera (~500 KB en float32).
    Dissenyada per a funcionar bé en EMNIST Letters i ser ràpida en CPU ARM.
    """
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(IMG_SIZE, IMG_SIZE, 1)),

        # Bloc 1
        tf.keras.layers.Conv2D(32, 3, padding='same', activation='relu'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling2D(2),       # 14×14

        # Bloc 2
        tf.keras.layers.Conv2D(64, 3, padding='same', activation='relu'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling2D(2),       # 7×7

        # Bloc 3
        tf.keras.layers.Conv2D(64, 3, padding='same', activation='relu'),
        tf.keras.layers.BatchNormalization(),

        tf.keras.layers.Flatten(),
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.4),
        tf.keras.layers.Dense(NUM_CLASSES, activation='softmax'),
    ], name="emnist_cnn")

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy'],
    )
    model.summary()
    return model


# ── Entrenament ───────────────────────────────────────────────────────────────

def train(model, ds_train, ds_test):
    callbacks = [
        tf.keras.callbacks.EarlyStopping(
            monitor='val_accuracy', patience=3, restore_best_weights=True
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            monitor='val_loss', factor=0.5, patience=2, min_lr=1e-5
        ),
    ]

    print(f"\nEntrenant {EPOCHS} epochs màx (early stopping actiu)...")
    history = model.fit(
        ds_train,
        validation_data=ds_test,
        epochs=EPOCHS,
        callbacks=callbacks,
    )

    _, acc = model.evaluate(ds_test, verbose=0)
    print(f"\nAccuracy final en test: {acc*100:.2f}%")
    return history


# ── Exportació a TFLite ───────────────────────────────────────────────────────

def export_tflite(model, ds_train_raw):
    """
    Exporta el model amb quantització dinàmica (DEFAULT).
    Redueix la mida ~4× i accelera la inferència en CPU ARM sense pèrdua apreciable.
    """
    print("\nExportant a TFLite...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    tflite_model = converter.convert()

    with open(MODEL_PATH, 'wb') as f:
        f.write(tflite_model)

    size_kb = os.path.getsize(MODEL_PATH) / 1024
    print(f"Model guardat: {MODEL_PATH}  ({size_kb:.0f} KB)")


# ── Verificació local ─────────────────────────────────────────────────────────

def verify_tflite(ds_test):
    """Comprova que el .tflite funciona i mostra la precisió real."""
    print("\nVerificant el .tflite...")
    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
    interpreter.allocate_tensors()

    inp  = interpreter.get_input_details()[0]
    out  = interpreter.get_output_details()[0]

    correct = total = 0
    for imgs, labels in ds_test.take(20):      # 20 batches de 256 = 5120 imatges
        for img, label in zip(imgs.numpy(), labels.numpy()):
            interpreter.set_tensor(inp['index'], img[np.newaxis].astype(np.float32))
            interpreter.invoke()
            pred = np.argmax(interpreter.get_tensor(out['index']))
            correct += (pred == label)
            total   += 1

    print(f"Precisió TFLite en {total} mostres: {correct/total*100:.2f}%")
    print(f"\n✓ Copia '{MODEL_PATH}' a la Raspberry:")
    print(f"  scp {MODEL_PATH} pi@<IP_RASPBERRY>:~/")


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == '__main__':
    ds_train, ds_test, ds_train_raw = load_dataset()
    model = build_model()
    train(model, ds_train, ds_test)
    export_tflite(model, ds_train_raw)
    verify_tflite(ds_test)