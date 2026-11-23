import numpy as np


def relu(x):
    return np.maximum(x, 0)


# Deep learning convolutional operator (stride = 1)
def conv2d(input, weights):
    K = weights.shape[0]  # Assuming square kernel
    N = input.shape[0]
    H_out = input.shape[1] - K + 1
    W_out = input.shape[2] - K + 1
    C_out = weights.shape[3]
    output = np.empty((N, H_out, W_out, C_out), dtype=np.float32)

    # Loop structure adapted from https://github.com/SkalskiP/ILearnDeepLearning.py/blob/ba0b5ba589d4e656141995e8d1a06d44db6ce58d/01_mysteries_of_neural_networks/06_numpy_convolutional_neural_net/src/layers/convolutional.py#L88
    for i in range(H_out):
        for j in range(W_out):
            output[:, i, j, :] = np.sum(
                input[:, i:i + K, j:j + K, :, np.newaxis] *
                weights[np.newaxis, :, :, :],
                axis=(1, 2, 3),
            )

    return output


# 2x2 maxpool operator, as used in LeNet-5
def maxpool2d(x):
    output = np.empty(
        [x.shape[0], x.shape[1] // 2, x.shape[2] // 2, x.shape[3]],
        dtype=x.dtype)
    for i in range(x.shape[1] // 2):
        for j in range(x.shape[2] // 2):
            output[:, i, j, :] = np.max(x[:, 2 * i:2 * i + 2,
                                          2 * j:2 * j + 2, :],
                                        axis=(1, 2))
    return output


# LeNet-5 Convolutional Neural Network (inference mode)
def lenet5(input, conv1, conv1bias, conv2, conv2bias, fc1w, fc1b, fc2w, fc2b,
           fc3w, fc3b, N, C_before_fc1):
    x = relu(conv2d(input, conv1) + conv1bias)
    x = maxpool2d(x)
    x = relu(conv2d(x, conv2) + conv2bias)
    x = maxpool2d(x)
    x = np.reshape(x, (N, C_before_fc1))
    x = relu(x @ fc1w + fc1b)
    x = relu(x @ fc2w + fc2b)
    return x @ fc3w + fc3b
