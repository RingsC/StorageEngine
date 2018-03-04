/**************************************************************************
* @file thread_commu_hs.h
* @brief 
* @author 黄晟
* @date 2011-10-31 10:22:15
* @version 1.0
**************************************************************************/



/************************************************************************** 
* @test_thread_communicate_twothread 
* 测试两个线程发信号，对应的两个线程接收信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_twothread( void );


/************************************************************************** 
* @brief test_thread_communicate_Onesend_Multreceive
* 测试一个线程发不同的信号，三个线程接收其信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_Onesend_Multreceive( void );


/************************************************************************** 
* @brief test_thread_communicate_Multsend_Onereceive
* 测试三个线程分别发不同的信号，一个线程接收这些信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_Multsend_Onereceive( void );


/************************************************************************** 
* @brief test_thread_communicate_Largesend
* 一个线程对另一个线程发多个信号，检测是否这些信号都能收到
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_Largesend(void);

/************************************************************************** 
* @brief test_thread_communicate_RandDely
* 测试两个线程与两个线程通信前，每个线程发生随机延迟，是否能够正确解释信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_RandDely(void);

/************************************************************************** 
* @brief test_thread_communicate_RandDely
* 测试两个线程与两个线程通信中，线程发生随机延迟，是否能够正确解释信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate_RandDely_thread(void);