#include "workthread.h"
#include <QMutexLocker>
#include <QFile>
#include <QDir>
//------------------------------------------------------------------------------
WorkThread::WorkThread() :
    QThread(nullptr)    // наш поток должен быть без предка чтобы перенестись сам в себя
{
    // Переносим СЕБЯ в ЭТОТ поток.
    // Это необходимо для корректной отправки сигналов -
    // они будут преобразованы в сообщения и перенесены в поток,
    // содержащий объекты со слотами, присоединенными к нашим сигналам.
    // Наш поток не может обрабатывать слоты, так как не запущена обработка сообщений.
    moveToThread( this );
}
//------------------------------------------------------------------------------
WorkThread::~WorkThread()
{

}
//------------------------------------------------------------------------------
// Функция добавляет новое задание для потока
void WorkThread::addTask(const QString &fileName)
{
    Task t;
    t.fileName = fileName;      // оборачиваем параметры задания структурой

    taskMutex.lock();
    taskQueue.push_back( t );   // помещаем задание в очередь
    taskMutex.unlock();
}
//------------------------------------------------------------------------------
// Функция переносит задания из входной очереди в рабочую.
// Перенос блокирует входную очередь, но происходит быстро;
// обработка - долго, но ничего не блокирует - поэтому нужны две очереди и перенос.
// Функция должна вызываться только из потока.
void WorkThread::moveTasksToWorkQueue()
{
    QMutexLocker locker( &taskMutex );

    try {
        while( !errorFlag &&
               !taskQueue.isEmpty() &&
               isRunning() )
        {
            workQueue.push_back( taskQueue.takeFirst() );
        }
    }
    catch(...) {
        errorFlag = true;
    }
}
//------------------------------------------------------------------------------
// Функция возвращает true если и рабочая и входная очереди пусты.
// Функция должна вызываться только из потока.
bool WorkThread::shouldSleepForAWhile()
{
    bool result;

    // работа со входной очередью допустима только через блокировку
    taskMutex.lock();
    result = taskQueue.isEmpty();
    taskMutex.unlock();

    return result && workQueue.isEmpty(); // состояние рабочей очереди можно получить без блокировки
}
//------------------------------------------------------------------------------
void WorkThread::processWorkQueue()
{
    ProcessingResult    processingResult;
    QList<Task>         notProcessed;
    Task                task;

    while( !errorFlag &&
           !workQueue.isEmpty() &&
           isRunning() )
    {
        // берем первое задание в очереди (убираем из очереди)
        task = workQueue.takeFirst();
        // обработка
        processingResult =  processTask( task );
        if( processingResult == prNotDoneYet ) {
            // если задание не обработано, то помещаем его в список
            notProcessed.push_back( task );
        }
        // если результатом обработки будет prDone, prImposible или prError,
        // то задание в очередь не вернется
    }
    // добавляем в рабочую очередь (пока что) необработанные задания
    workQueue.append( notProcessed );
}
//------------------------------------------------------------------------------
void WorkThread::run()
{
    errorFlag = false;
    do {
     // перенос в рабочую очередь
        moveTasksToWorkQueue();

     // обработка полученных заданий
        processWorkQueue();

     // После завершения заданий поток нужно усыпить,
     // если же заданий нет - спим, пока они не появятся.
     // Если этого не делать, то поток будет крутиться,
     // потребляя ресурсы (можно посмотреть в диспетчере задач)
        do {
            msleep(10); // поток засыпает на 10 милисекунд
        } while( !errorFlag &&
                 shouldSleepForAWhile() &&    // функция проверки очередей
                 isRunning() );
    } while( !errorFlag && isRunning() );
}
//==============================================================================
// обработка задания
WorkThread::ProcessingResult WorkThread::processTask(const WorkThread::Task &t)
{
    ProcessingResult result = prNotDoneYet;

    try {
        uint    threadId = uint(QThread::currentThreadId()); // идентификатор потока, в котором выполняется log

        if( QFile::exists(t.fileName) ) {
            QFile   file(t.fileName);

            if( file.open(QIODevice::ReadOnly) ) {
             // файл готов к обработке
                emit outMessage( QString("+++(0x%2) file \"%1\" has been processed+++")
                                        .arg( QDir::toNativeSeparators(t.fileName) )
                                        .arg( threadId, 0, 16 )
                               );
                result = prDone;
            }
        } else {
         // файл обработать невозможно....
            emit outMessage( QString("---(0x%2) processing is impossible for \"%1\"---")
                                    .arg( QDir::toNativeSeparators(t.fileName) )
                                    .arg( threadId, 0, 16 )
                           );
            result = prImposible;
        }
    }
    catch(...) {
        errorFlag = true;
        result = prError;
    }
    return result;
}
//==============================================================================
