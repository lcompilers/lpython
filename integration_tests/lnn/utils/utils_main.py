from lpython import i32, f64, InOut

def normalize(value: f64, leftMin: f64, leftMax: f64, rightMin: f64, rightMax: f64) -> f64:
    # Figure out how 'wide' each range is
    leftSpan: f64 = leftMax - leftMin
    rightSpan: f64 = rightMax - rightMin

    # Convert the left range into a 0-1 range (float)
    valueScaled: f64 = (value - leftMin) / leftSpan

    # Convert the 0-1 range into a value in the right range.
    return rightMin + (valueScaled * rightSpan)

def normalize_input_vectors(input_vectors: InOut[list[list[f64]]]):
    rows: i32 = len(input_vectors)
    cols: i32 = len(input_vectors[0])

    j: i32
    for j in range(cols):
        colMinVal: f64 = input_vectors[0][j]
        colMaxVal: f64 = input_vectors[0][j]
        i: i32
        for i in range(rows):
            if input_vectors[i][j] > colMaxVal:
                colMaxVal = input_vectors[i][j]
            if input_vectors[i][j] < colMinVal:
                colMinVal = input_vectors[i][j]

        for i in range(rows):
            input_vectors[i][j] = normalize(input_vectors[i][j], colMinVal, colMaxVal, -1.0, 1.0)

def normalize_output_vector(output_vector: InOut[list[f64]]):
    rows: i32 = len(output_vector)
    colMinVal: f64 = output_vector[0]
    colMaxVal: f64 = output_vector[0]
    i: i32
    for i in range(rows):
        if output_vector[i] > colMaxVal:
            colMaxVal = output_vector[i]
        if output_vector[i] < colMinVal:
            colMinVal = output_vector[i]

    for i in range(rows):
        output_vector[i] = normalize(output_vector[i], colMinVal, colMaxVal, -1.0, 1.0)
