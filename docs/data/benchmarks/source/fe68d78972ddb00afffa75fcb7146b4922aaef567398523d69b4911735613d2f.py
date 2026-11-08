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


# Batch normalization operator, as used in ResNet
def batchnorm2d(x, eps=1e-5):
    mean = np.mean(x, axis=0, keepdims=True)
    std = np.std(x, axis=0, keepdims=True)
    return (x - mean) / np.sqrt(std + eps)


# Bottleneck residual block (after initial convolution, without downsampling)
# in the ResNet-50 CNN (inference)
def resnet_basicblock(input, conv1, conv2, conv3):
    # Pad output of first convolution for second convolution
    padded = np.zeros((input.shape[0], input.shape[1] + 2, input.shape[2] + 2,
                       conv1.shape[3]))

    padded[:, 1:-1, 1:-1, :] = conv2d(input, conv1)
    x = batchnorm2d(padded)
    x = relu(x)

    x = conv2d(x, conv2)
    x = batchnorm2d(x)
    x = relu(x)
    x = conv2d(x, conv3)
    x = batchnorm2d(x)
    return relu(x + input)
