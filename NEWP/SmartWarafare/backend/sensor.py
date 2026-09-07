"""Shared sensor characteristics for the demo Electronic Support receiver.

Kept in one place so the live app, the offline training-data generator,
and the scheduler comparison script all model the same noisy detector.
"""

SENSOR_PD = 0.92    # prob. of detecting a real transmission on the scanned band
SENSOR_PFA = 0.03   # prob. of a false alarm when the scanned band is silent
