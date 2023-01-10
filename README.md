# TEEServer



`Client/client.cpp` 中是客户端的代码，目前写的是一个客户端构造一个长度是100的向量，然后发给TEE。

`App/App.cpp`中是`server`的代码，其中`server`的一些参数在这段代码：

``` c
const char *ip = "127.0.0.1";
int port = 30000;
int num_of_clients = 2;
```

目前实现的是两个client发送自己的向量到TEEServer，TEEServer进行运算然后返回

其中这段代码是TEEServer的主要部分

``` c
/* 这个相当于一个二位数组，第一位存储第几个client ，第二位存储这个client对应的向量，类型是int* */
int **vec_arr = (int **)malloc(num_of_clients * sizeof(int *));

/* 这个存储所有client的socket */
int client_sockets[num_of_clients];

for (int i = 0; i < num_of_clients; i++) {
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                               &client_addr_size);
    printf("Receive Client %d's data.\n", i);
    int strlen = recv(client_socket, buffer, 400, 0);
	/* 现在buffer中就存储了本次client发送的向量 */
    int *vec = (int *)malloc(100 * sizeof(int));
	/* 把buffer的内容拷贝到vec中 */
    memcpy(vec, buffer, 400);
	/* 第i个client对应的向量就是vec */
    vec_arr[i] = vec;

    client_sockets[i] = client_socket;

    /* Reset buffer */
    memset(buffer, 0, 400);
}
/* res存储计算结果，计算在TEE中进行 */
int res;
/* 调用这个函数，在TEE中运算，参数解释如下：
* global_eid:不用管，直接放这里
* &res:传进去之后，函数内算完结果会把结果存在这个指针
* vec_arr：类型是int **，具体什么含义上面解释过
* num_of_clients: TEEServer需要支持几个客户端
*/
ecall_vecmul(global_eid, &res, vec_arr, num_of_clients);

  /* Send the result back to all clients */
for (int i = 0; i < num_of_clients; i++) {
    send(client_sockets[i], (char *)&res, 4, 0);
    close(client_sockets[i]);
}

```

现在需要实现的是：

* 一共3个client，每个client发送自己的3个向量到TEEServer。
* TEEServer需要把这些share恢复成原始的三个向量，然后调用上面的框架进行运算，返回结果。
* 恢复的代码可以直接在`App/App.cpp`中写。