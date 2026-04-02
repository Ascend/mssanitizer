# MindStudio Sanitizer 对外接口使用说明

<br>

## 接口列表

**接口简介**

msSanitizer工具包含sanitizer接口和mstx扩展接口两种类型。sanitizer接口用于CANN软件栈的检测，与ACL系列接口一一对应。此类接口会在ACL对应接口的功能基础上，额外向工具上报接口调用位置的代码文件和行号信息，使用时需导入sanitizer API头文件和链接动态库，具体请参见《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>导入API头文件和链接动态库”。mstx扩展接口用于用户自定义上报内存池信息，以实现更准确的检测，具体请参见[mstx扩展功能](#mstx扩展功能)。

**表 1**  msSanitizer工具接口列表

|接口名称|功能简介|
|--|--|
|[sanitizer接口](#sanitizer接口)|在ACL对应接口的功能基础上，向msSanitizer工具上报sanitizer接口调用位置的代码文件和行号信息。|
|[sanitizerRtMalloc](#sanitizerrtmalloc)|调用aclrtMalloc接口在Device上分配size大小的线性内存，并通过*devPtr返回已分配内存的指针，并向检测工具上报内存分配信息。实际的内存分配行为和参数含义与aclrtMalloc一致。|
|[sanitizerRtMallocCached](#sanitizerrtmalloccached)|调用aclrtMallocCached接口在Device上申请size大小的线性内存，通过*devPtr返回已分配内存的指针，并向检测工具上报内存分配信息。该接口在任何场景下，申请的内存都支持cache缓存。实际的内存分配行为和参数含义与aclrtMallocCached一致。|
|[sanitizerRtFree](#sanitizerrtfree)|调用aclrtFree接口释放Device上的内存，并向检测工具上报内存释放信息。实际的内存释放行为和参数含义与aclrtFree一致。|
|[sanitizerRtMemset](#sanitizerrtmemset)|调用aclrtMemset接口初始化内存，将内存中的内容设置为指定值，并向检测工具上报内存初始化信息。实际的内存初始化行为和参数含义与aclrtMemset一致。|
|[sanitizerRtMemsetAsync](#sanitizerrtmemsetasync)|调用aclrtMemsetAsync接口初始化内存，将内存中的内容设置为指定的值，并向检测工具上报内存初始化信息。此接口为异步接口。实际的内存初始化行为和参数含义与aclrtMemsetAsync一致。|
|[sanitizerRtMemcpy](#sanitizerrtmemcpy)|调用aclrtMemcpy接口完成内存复制，并向检测工具上报内存复制信息。实际的内存复制行为和参数含义与aclrtMemcpy一致。|
|[sanitizerRtMemcpyAsync](#sanitizerrtmemcpyasync)|调用aclrtMemcpyAsync接口完成内存复制，并向检测工具上报内存复制信息。此接口为异步接口。实际的内存复制行为和参数含义与aclrtMemcpyAsync一致。|
|[sanitizerRtMemcpy2d](#sanitizerrtmemcpy2d)|调用aclrtMemcpy2d接口完成矩阵数据内存复制，并向检测工具上报内存复制信息。实际的矩阵数据内存复制行为和参数含义与aclrtMemcpy2d一致。|
|[sanitizerRtMemcpy2dAsync](#sanitizerrtmemcpy2dasync)|调用aclrtMemcpy2dAsync接口完成矩阵数据内存复制，并向检测工具上报内存复制信息。此接口为异步接口。实际的矩阵数据内存复制行为和参数含义与aclrtMemcpy2dAsync一致。|
|[sanitizerReportMalloc](#sanitizerreportmalloc)|手动上报GM内存分配信息。|
|[sanitizerReportFree](#sanitizerreportfree)|手动上报GM内存释放信息。|
|[mstx扩展功能](#mstx扩展功能)|mstx接口是MindStudio提供的一套扩展接口，它允许用户在应用程序中插入特定的标记，以便在工具进行内存检测时能够更精确地定位特定算子的内存问题。|
|mstxDomainCreateA|创建域。|
|mstxMemHeapRegister|内存池注册接口。|
|mstxMemHeapUnregister|内存池注销接口。|
|mstxMemRegionsRegister|内存池二次分配注册接口。|
|mstxMemRegionsUnregister|内存池二次分配注销接口。|

## sanitizer接口

<br>

### sanitizerRtMalloc

---

**功能说明**

调用aclrtMalloc接口在Device上分配size大小的线性内存，并通过*devPtr返回已分配内存的指针，并向检测工具上报内存分配信息。实际的内存分配行为和参数含义与aclrtMalloc一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMalloc的详细说明。

**函数原型**

```c
aclError sanitizerRtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|devPtr|输出|指向“Device上已分配内存的指针”的指针。|
|size|输入|申请内存的大小，单位为Byte。size不能为0。|
|policy|输入|内存分配规则。|
|filename|输入|内存分配被调用处的文件名。|
|lineno|输入|内存分配被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMallocCached

---

**功能说明**

调用aclrtMallocCached接口在Device上申请size大小的线性内存，通过*devPtr返回已分配内存的指针，并向检测工具上报内存分配信息。该接口在任何场景下，申请的内存都支持cache缓存。实际的内存分配行为和参数含义与aclrtMallocCached一致。

> [!NOTE] 说明        
> 可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节章节查看aclrtMallocCached的详细说明。

**函数原型**

```c
aclError sanitizerRtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|devPtr|输出|指向“Device上已分配内存的指针”的指针。|
|size|输入|申请内存的大小，单位为Byte。size不能为0。|
|policy|输入|内存分配规则。|
|filename|输入|内存分配被调用处的文件名。|
|lineno|输入|内存分配被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtFree

---

**功能说明**

调用aclrtFree接口释放Device上的内存，并向检测工具上报内存释放信息。实际的内存释放行为和参数含义与aclrtFree一致。

>[!NOTE] 说明 
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtFree的详细说明。

**函数原型**

```c
aclError sanitizerRtFree(void *devPtr, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|devPtr|输入|待释放内存的指针。|
|filename|输入|内存释放被调用处的文件名。|
|lineno|输入|内存释放被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemset

---

**功能说明**

调用aclrtMemset接口初始化内存，将内存中的内容设置为指定值，并向检测工具上报内存初始化信息。实际的内存初始化行为和参数含义与aclrtMemset一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemset的详细说明。

**函数原型**

```c
aclError sanitizerRtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|devPtr|输入|内存起始地址的指针。|
|maxCount|输入|内存的最大长度，单位为Byte。|
|value|输入|初始化内存的指定值。|
|count|输入|需要设置为指定值的内存长度，单位为Byte。|
|filename|输入|内存初始化被调用处的文件名。|
|lineno|输入|内存初始化被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemsetAsync

---

**功能说明**

调用aclrtMemsetAsync接口初始化内存，将内存中的内容设置为指定的值，并向检测工具上报内存初始化信息。此接口为异步接口。实际的内存初始化行为和参数含义与aclrtMemsetAsync一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemsetAsync的详细说明。

**函数原型**

```c
aclError sanitizerRtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|devPtr|输入|内存起始地址的指针。|
|maxCount|输入|内存的最大长度，单位为Byte。|
|value|输入|初始化内存的指定值。|
|count|输入|初始化内存的长度，单位为Byte。|
|stream|输入|指定的stream。|
|filename|输入|内存初始化被调用处的文件名。|
|lineno|输入|内存初始化被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemcpy

---

**功能说明**

调用aclrtMemcpy接口完成内存复制，并向检测工具上报内存复制信息。实际的内存复制行为和参数含义与aclrtMemcpy一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemcpy的详细说明。

**函数原型**

```c
aclError sanitizerRtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|dst|输入|目的内存地址指针。|
|destMax|输入|目的内存地址的最大内存长度，单位为Byte。|
|src|输入|源内存地址指针。|
|count|输入|内存复制的长度，单位为Byte。|
|kind|输入|预留参数，系统内部会根据源内存地址指针、目的内存地址指针判断是否可以将源地址的数据复制到目的地址，如果不可以，则系统会返回报错。|
|filename|输入|内存复制被调用处的文件名。|
|lineno|输入|内存复制被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemcpyAsync

---

**功能说明**

调用aclrtMemcpyAsync接口完成内存复制，并向检测工具上报内存复制信息。此接口为异步接口。实际的内存复制行为和参数含义与aclrtMemcpyAsync一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemcpyAsync的详细说明。

**函数原型**

```c
aclError sanitizerRtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|dst|输入|目的内存地址指针。|
|destMax|输入|目的内存地址的最大内存长度，单位为Byte。|
|src|输入|源内存地址指针。|
|count|输入|内存复制的长度，单位为Byte。|
|kind|输入|预留参数，系统内部会根据源内存地址指针、目的内存地址指针判断是否可以将源地址的数据复制到目的地址，如果不可以，则系统会返回报错。|
|stream|输入|当前内存复制行为指定的stream。|
|filename|输入|内存复制被调用处的文件名。|
|lineno|输入|内存复制被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemcpy2d

---

**功能说明**

调用aclrtMemcpy2d接口完成矩阵数据内存复制，并向检测工具上报内存复制信息。实际的矩阵数据内存复制行为和参数含义与aclrtMemcpy2d一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemcpy2d的详细说明。

**函数原型**

```c
aclError sanitizerRtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, aclrtMemcpyKind kind, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|dst|输入|目的内存地址指针。|
|dpitch|输入|目的内存中相邻两列向量的地址距离。|
|src|输入|源内存地址指针。|
|spitch|输入|源内存中相邻两列向量的地址距离。|
|width|输入|待复制的矩阵宽度。|
|height|输入|待复制的矩阵高度。height最大设置为5*1024*1024=5242880，否则接口返回失败。|
|kind|输入|内存复制的类型。|
|filename|输入|矩阵数据内存复制被调用处的文件名。|
|lineno|输入|矩阵数据内存复制被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerRtMemcpy2dAsync

---

**功能说明**

调用aclrtMemcpy2dAsync接口完成矩阵数据内存复制，并向检测工具上报内存复制信息。此接口为异步接口。实际的矩阵数据内存复制行为和参数含义与aclrtMemcpy2dAsync一致。

>[!NOTE] 说明
>可参见《[应用开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html)》手册中“acl API参考（C） > 运行时管理 > 内存管理”章节查看aclrtMemcpy2dAsync的详细说明。

**函数原型**

```c
aclError sanitizerRtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
```

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|dst|输入|目的内存地址指针。|
|dpitch|输入|目的内存中相邻两列向量的地址距离。|
|src|输入|源内存地址指针。|
|spitch|输入|源内存中相邻两列向量的地址距离。|
|width|输入|待复制的矩阵宽度。|
|height|输入|待复制的矩阵高度。height最大设置为5*1024*1024=5242880，否则接口返回失败。|
|kind|输入|内存复制的类型。|
|stream|输入|当前矩阵数据内存复制行为指定的stream。|
|filename|输入|矩阵数据内存复制被调用处的文件名。|
|lineno|输入|矩阵数据内存复制被调用处的行号。|

**返回值**

返回0表示成功，返回其它值表示失败。

**调用示例**

具体操作请参见使用示例的《[MindStudio Sanitizer典型案例](../best_practices/basic_cases.md)》中的“检测CANN软件栈的内存>排查步骤里的步骤4”。

<br>
<br>

### sanitizerReportMalloc

---

**功能说明**

手动上报GM内存分配信息。

**函数原型**

```c
void sanitizerReportMalloc(void *ptr, uint64_t size);
```

> [!NOTE] 说明      
> 此接口是__sanitizer_report_malloc接口的封装，__sanitizer_report_malloc接口为弱函数，只有当用户程序被检测工具拉起时才会生效。

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|ptr|输入|分配的内存地址。|
|size|输入|分配的内存长度。|

**返回值**

无

**调用示例**

无

<br>
<br>

### sanitizerReportFree

---

**功能说明**

手动上报GM内存释放信息。

**函数原型**

```c
void sanitizerReportFree(void *ptr);
```

>[!NOTE] 说明   
>此接口是__sanitizer_report_free接口的封装，__sanitizer_report_free接口为弱函数，只有当用户程序被检测工具拉起时才会生效。

**参数说明**

**表 1**  参数说明

|参数名|输入/输出|描述|
|--|--|--|
|ptr|输入|释放的内存地址。|

**返回值**

无

**调用示例**

无

## mstx扩展功能

**mstx接口简介**

mstx接口是MindStudio提供的一套扩展接口，它允许用户在应用程序中插入特定的标记，以便在工具进行内存检测时能够更精确地定位特定算子的内存问题。例如，针对二级指针类算子，在不使能mstx接口的情况下，得到的地址空间可能不准确。通过《[MindStudio Tools Extension Library接口文档](https://gitcode.com/Ascend/mstx/blob/master/docs/zh/api_reference/mstx_api_reference.md)》的mstxMemRegionsRegister和mstxMemRegionsUnregister接口，可以将准确的地址空间传递给异常检测工具，实现更精准的内存检测。

> [!NOTE] 说明      
>《[MindStudio Sanitizer工具用户指南](../user_guide/mssanitizer_user_guide.md)》中的“异常检测功能介绍>功能说明>调用场景>Kernel直调算子开发”中的内核调用符场景暂不支持使用mstx接口。

**mstx接口列表**

msSanitizer工具调用的mstx接口列表如[表1](#table111)所示，具体使用状况请参考《mstx_api》。

**表 1**  msSanitizer工具调用的mstx接口列表<a name="table111"></a>

|接口名称|功能简介|
|--|--|
|mstxDomainCreateA|创建一个新的mstx域。|
|mstxMemHeapRegister|注册内存池。用户在调用该接口注册内存池时，需确保该内存已提前申请。|
|mstxMemRegionsRegister|注册内存池二次分配。用户需保证RegionsRegister的内存位于mstxMemHeapRegister注册的范围内，否则工具会提示越界读写。|
|mstxMemRegionsUnregister|注销内存池二次分配。|
|mstxMemHeapUnregister|注销内存池时，与之关联的Regions将一并被注销。|

**mstx接口的使用**

- msSanitizer工具默认使能mstx接口，允许用户使用mstx接口自定义算子使用的内存空间地址和大小，可识别并快速界定算子的内存问题。
- mstx当前提供了两种API的使用方式：库文件和头文件，以[AclNNInvocation中代码](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation)为例：

    >[!NOTE] 说明  
    > 此样例工程不支持Atlas A3 训练系列产品/Atlas A3 推理系列产品。

- 在${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/CMakeLists.txt路径下新增库文件libms_tools_ext.so，地址为：${INSTALL_DIR}/lib64/libms_tools_ext.so。
    
    ```c  
        # Header path
        include_directories(
             ...
            ${CUST_PKG_PATH}/include
        )
        ...
        target_link_libraries( 
            ...
            dl
        )

    ```

- 在${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/main.cpp路径下，将用户程序编译链接dl库，对应的头文件ms_tools_ext.h地址：${INSTALL_DIR}/include/mstx。
    
    ```c
    ...
    #include "mstx/ms_tools_ext.h"
    ...
    ```

    > [!NOTE] 说明        
    > ${INSTALL_DIR}请替换为CANN软件安装后文件存储路径。以root用户安装为例，安装后文件默认存储路径为：/usr/local/Ascend/cann。 

**调用示例**

```c
mstxMemVirtualRangeDesc_t rangeDesc = {};
    rangeDesc.deviceId = deviceId;       // 设备编号
    rangeDesc.ptr = gm;                  // 注册的内存池CM首地址
    rangeDesc.size = 1024;               // 内存池大小
    heapDesc.typeSpecificDesc = &rangeDesc;
    mstxMemHeapDesc_t heapDesc{};
    mstxMemHeapHandle_t memPool = mstxMemHeapRegister(globalDomain, &heapDesc); // 注册内存池
    mstxMemVirtualRangeDesc_t rangesDesc[1] = {};                // 二次分配包含的region数量
    mstxMemRegionHandle_t regionHandles[1] = {};
    rangesDesc[0].deviceId = deviceId;                           // 设备编号
    rangesDesc[0].ptr = gm;                                      // 二次分配GM地址
    rangesDesc[0].size = 256;                                    // 二次分配大小
    mstxMemRegionsRegisterBatch_t regionsDesc{};
    regionsDesc.heap = memPool;
    regionsDesc.regionType = MSTX_MEM_TYPE_VIRTUAL_ADDRESS;
    regionsDesc.regionCount = 1;
    regionsDesc.regionDescArray = rangesDesc;
    regionsDesc.regionHandleArrayOut = regionHandles;
    mstxMemRegionsRegister(globalDomain, regionsDesc);              // 二次分配注册
    Do(blockDim, nullptr, stream, gm);                            // 算子Kernel函数
    mstxMemRegionRef_t regionRef[1] = {};
    regionRef[0].refType = MSTX_MEM_REGION_REF_TYPE_HANDLE;
    regionRef[0].handle = regionHandles[0];
    mstxMemRegionsUnregisterBatch_t refsDesc = {};
    refsDesc.refCount = 1;
    refsDesc.refArray = regionRef;
    mstxMemRegionsUnregister(globalDomain, &refsDesc);                   // 注销二次分配
    mstxMemHeapUnregister(globalDomain, memPool);                        // 注销内存池
```
