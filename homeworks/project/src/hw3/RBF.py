import sys
import numpy as np
import matplotlib.pyplot as plt
import os
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"   # 指定使用CPU训练


# 径向高斯基函数
class RBF:
    # 初始化学习率、学习步数
    def __init__(self, learning_rate=0.002, step_num=10001, hidden_size=10):
        self.learning_rate = learning_rate
        self.step_num = step_num
        self.hidden_size = hidden_size
        self.feature = []
        self.a = []
        self.b = []
        self.w = []
        self.w_0 = []

    # 高斯基函数
    def kernel(self, _x, a, b):  # 训练时使用
        x1 = tf.tile(_x, [1, self.hidden_size])  # 将x水平复制 hidden次
        x2 = tf.reshape(x1, [-1, self.hidden_size, self.feature])
        dist = tf.reduce_sum((a * x2 + b) ** 2, 2)
        return tf.exp(-dist / 2)

    # 训练RBF神经网络
    def train(self, _x, _y):
        self.feature = np.shape(_x)[1]  # 输入值的特征数
        x_ = tf.placeholder(tf.float32, [None, self.feature])  # 定义placeholder
        y_ = tf.placeholder(tf.float32, [None, 1])  # 定义placeholder

        # 定义径向基层
        # a = tf.Variable(tf.random_normal([self.hidden_size, self.feature]))
        a = tf.Variable(tf.random_normal([self.hidden_size]))
        b = tf.Variable(tf.random_normal([self.hidden_size, self.feature]))
        z = self.kernel(x_, a, b)

        # 定义输出层
        w = tf.Variable(tf.random_normal([self.hidden_size, 1]))
        w_0 = tf.Variable(tf.zeros([1]))
        yf = tf.matmul(z, w) + w_0

        loss = tf.reduce_mean(tf.square(y_ - yf))  # 定义损失函数
        optimizer = tf.train.AdamOptimizer(self.learning_rate)  # ADAM优化
        train = optimizer.minimize(loss)  # 优化损失函数
        init = tf.global_variables_initializer()  # 变量初始化

        with tf.Session() as sess:
            sess.run(init)
            for epoch in range(self.step_num):
                sess.run(train, feed_dict={x_: _x, y_: _y})
                if epoch > 0 and epoch % 500 == 0:
                    mse = sess.run(loss, feed_dict={x_: _x, y_: _y})
                    print('epoch:', epoch, '\t loss:', mse)
            self.a, self.b, self.w, self.w_0 = sess.run([a, b, w, w_0], feed_dict={x_: _x, y_: _y})

    def kernel2(self, _x, a, b):  # 预测时使用
        x1 = np.tile(_x, [1, self.hidden_size])  # 将x水平复制 hidden次
        x2 = np.reshape(x1, [-1, self.hidden_size, self.feature])
        dist = np.sum((a * x2 + b) ** 2, 2)
        return np.exp(-dist / 2)

    def predict(self, _x):
        z = self.kernel2(_x, self.a, self.b)
        _pre = np.matmul(z, self.w) + self.w_0
        return _pre


if __name__ == '__main__':
    # 从文件读取数据点tx
    data = np.loadtxt('tx.txt')
    data1 = np.loadtxt('ty.txt')
    x = data[:, 0]
    x1 = data1[:, 0]
    y = data[:, 1]
    y1 = data1[:, 1]
    x = np.reshape(x, (len(x), 1))
    x1 = np.reshape(x1, (len(x1), 1))
    y = np.reshape(y, (len(y), 1))
    y1 = np.reshape(y1, (len(y1), 1))

    s = max((max(y)-min(y)), (max(y1)-min(y1)))

    y = (y-min(y)) / s
    y1 = (y1-min(y1)) / s


    # RBF
    rate = float(sys.argv[1])
    step = int(sys.argv[2])
    layer = int(sys.argv[3])
    rbf = RBF(rate, step, hidden_size=layer)  # 学习率
    rbf1 = RBF(rate, step, hidden_size=layer)  # 学习率
    rbf.train(x, y)
    rbf1.train(x1, y1)

    # 测试数据x
    t = np.arange(0, 1, 0.001)
    t = np.reshape(t, (len(t), 1))
    rbf_pre_x = rbf.predict(t)    # RBF预测
    rbf_pre_y = rbf1.predict(t)  # RBF预测

    plt.plot(y, y1, 'k*')
    plt.plot(rbf_pre_x, rbf_pre_y, 'c')
    plt.gca().invert_yaxis()
    # plt.legend(['origin data', 'RBF'], loc='upper left')
    plt.axis('equal')
    plt.axis('off')
    # plt.savefig("HW2_14.png", dpi=300)
    plt.show()
