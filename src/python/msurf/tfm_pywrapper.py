# src/python/msurf/tfm_pywrapper.py
import ctypes
import numpy as np
import os

# Load the shared library from site-packages/msurf/
lib_path = os.path.join(os.path.dirname(__file__), "libtfm_pywrapper.cpython-311-darwin.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError(f"Could not find {lib_path}. Ensure 'pip install .' installed the library correctly.")

tfm_lib = ctypes.CDLL(lib_path)
# Define fp_int structure
class fp_int(ctypes.Structure):
    _fields_ = [
        ("dp", ctypes.c_uint32 * 6),  # FP_SIZE = 192 / DIGIT_BIT (32) = 6
        ("used", ctypes.c_int),
        ("sign", ctypes.c_int)
    ]

# Function signatures
tfm_lib.fp_from_double.argtypes = [ctypes.POINTER(fp_int), ctypes.c_double]
tfm_lib.fp_from_double.restype = None

tfm_lib.fp_from_float.argtypes = [ctypes.POINTER(fp_int), ctypes.c_float]
tfm_lib.fp_from_float.restype = None

tfm_lib.fp_to_double.argtypes = [ctypes.POINTER(fp_int)]
tfm_lib.fp_to_double.restype = ctypes.c_double

tfm_lib.fp_to_float.argtypes = [ctypes.POINTER(fp_int)]
tfm_lib.fp_to_float.restype = ctypes.c_float

# Wrapper functions
def fp_from_double(value: np.float64) -> fp_int:
    """Convert numpy.float64 to fp_int."""
    if not isinstance(value, np.float64):
        value = np.float64(value)
    result = fp_int()
    tfm_lib.fp_from_double(ctypes.byref(result), value)
    return result

def fp_from_float(value: np.float32) -> fp_int:
    """Convert numpy.float32 to fp_int."""
    if not isinstance(value, np.float32):
        value = np.float32(value)
    result = fp_int()
    tfm_lib.fp_from_float(ctypes.byref(result), value)
    return result

def fp_to_double(num: fp_int) -> np.float64:
    """Convert fp_int to numpy.float64."""
    result = tfm_lib.fp_to_double(ctypes.byref(num))
    return np.float64(result)

def fp_to_float(num: fp_int) -> np.float32:
    """Convert fp_int to numpy.float32."""
    result = tfm_lib.fp_to_float(ctypes.byref(num))
    return np.float32(result)

# Test the wrapper
if __name__ == "__main__":
    # Test double
    val_double = np.float64(0.25)
    fp_val = fp_from_double(val_double)
    print(f"fp_from_double({val_double}): used={fp_val.used}, sign={fp_val.sign}, dp={list(fp_val.dp)}")
    back_double = fp_to_double(fp_val)
    print(f"fp_to_double: {back_double} (matches input: {abs(back_double - val_double) < 1e-10})")

    # Test float
    val_float = np.float32(0.52)
    fp_val = fp_from_float(val_float)
    print(f"fp_from_float({val_float}): used={fp_val.used}, sign={fp_val.sign}, dp={list(fp_val.dp)}")
    back_float = fp_to_float(fp_val)
    print(f"fp_to_float: {back_float} (matches input: {abs(back_float - val_float) < 1e-5})")
