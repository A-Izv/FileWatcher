#include "workthread.h"
#include <QMutexLocker>
#include <QFile>
#include <QDir>
#include "simpleippfir2file.h"
//------------------------------------------------------------------------------
WorkThread::WorkThread() :
    QThread(nullptr),   // наш поток должен быть без предка чтобы перенестись сам в себя
    taskMutex(nullptr)
{
/**/buf = ippsMalloc_32fc( BUFFER_SIZE_SMPL );

    taskMutex = new QMutex;

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
/**/ippsFree( buf );
/**/clearFilters();

    delete taskMutex;
}
//------------------------------------------------------------------------------
// Метод добавляет новое задание для потока
// !! ЭТОТ МЕТОД ВЫПОЛНЯЕТСЯ В ДРУГОМ ПОТОКЕ!
void WorkThread::addTask( const QString      &fileName,      // имя исходного файла
                          const QString      &outPath,       // каталог для сохранения результата
                          const QList<float> &frqs      )   // частоты
{
/**/ // оборачиваем параметры задания структурой
/**/    Task t;
/**/    t.fileName  = fileName;
/**/    t.outPath   = outPath;
/**/
/**/    for( auto it = frqs.cbegin() ; it != frqs.cend() ; ++it ) {
/**/        float f = *it;
/**/        if( f >= -0.5 && f < 0.5 ) {
/**/            t.frqs.append( f );
/**/        }
/**/    }
/**/
/**/ // контроль корректности переданных параметров
/**/    if( QFile::exists(fileName) &&
/**/        !frqs.isEmpty()         &&
/**/        QDir().mkpath( outPath ) ) // создаем и/или проверяем наличие директории
        {
         // помещаем задание в очередь
            taskMutex->lock();
            taskQueue.push_back( t );
            taskMutex->unlock();
        }
/**/    else
/**/    { // задание с переданными параметрами даже не стоит помещать в очередь
/**/        emit outMessage( QString( "Wrong params for file \"%1\"")
/**/                            .arg( QDir::toNativeSeparators(fileName) )
/**/                        );
/**/    }
}
//------------------------------------------------------------------------------
// Метод переносит задания из входной очереди в рабочую.
// Перенос блокирует входную очередь, но происходит быстро;
// обработка - долго, но ничего не блокирует - поэтому нужны две очереди и перенос.
// Метод должен вызываться только из этого потока.
void WorkThread::moveTasksToWorkQueue()
{
    QMutexLocker locker( taskMutex );

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
// Метод возвращает true если и рабочая и входная очереди пусты.
// Метод должен вызываться только из этого потока.
bool WorkThread::shouldSleepForAWhile()
{
    bool result;

    // работа со входной очередью допустима только через блокировку
    taskMutex->lock();
    result = taskQueue.isEmpty();
    taskMutex->unlock();

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
                 shouldSleepForAWhile() &&    // проверка очередей
                 isRunning() );
    } while( !errorFlag && isRunning() );
}
//******************************************************************************
// обработка задания
WorkThread::ProcessingResult WorkThread::processTask(const WorkThread::Task &t)
{
    ProcessingResult    result = prNotDoneYet;
    qint64              size;
    uint                threadId = uint(QThread::currentThreadId()); // идентификатор потока, в котором выполняется log

    if( QFile::exists(t.fileName) ) {
        QFile   file(t.fileName);

        try {
            if( file.open(QIODevice::ReadOnly) ) {
             // файл готов к обработке
                // подготовка к работе
                initFilters( t.outPath, t.fileName, t.frqs );
                // обработка файла
                while( !file.atEnd() &&
                       !errorFlag &&
                       isRunning() )
                {
                 // читаем порцию данных
                    size  = file.read( (char*)buf, BUFFER_SIZE_BYTE );
                    size /= sizeof(Ipp32fc);
                 // отправляем данные на обработку
                    filterPortion( buf, size );
                }
                // завершаем обработку
                clearFilters();
             // отправляем сообщение о готовности
                emit outMessage( QString("+++(0x%2) file \"%1\" has been processed+++")
                                        .arg( QDir::toNativeSeparators(t.fileName) )
                                        .arg( threadId, 0, 16 )
                               );
                result = prDone;
            }
        }
        catch(...) {
            errorFlag = true;
            result = prError;
        }
    } else {
     // файл обработать невозможно (его больше нет)....
        emit outMessage( QString("---(0x%2) processing is impossible for \"%1\"---")
                               .arg( QDir::toNativeSeparators(t.fileName) )
                               .arg( threadId, 0, 16 )
                      );
        result = prImposible;
    }
    return result;
}
//******************************************************************************
void WorkThread::initFilters(const QString &outPath,
                             const QString &srcFileName,
                             const QList<float> &frqs)
{
    SimpleIppFIR2File   *flt;
    QFileInfo           fi(srcFileName);
    QString             name = fi.completeBaseName();
    QString             ext  = fi.suffix();
    QString             str;
    float               f;

    for( auto it = frqs.cbegin() ; it != frqs.cend() ; ++it ) {
        f = *it;
     // формируем имя выходного файла
        if( f < 0 ) f += 1;
        str = outPath + "/" + name + "_" +
              QString::number(f,'f',2).replace('.','_') +
              "." +  ext;
     // создаем фильтр и помещаем его в список обработчиков
        flt = new SimpleIppFIR2File( str, f, BUFFER_SIZE_SMPL );
        if( flt->hasError() ) {
            delete flt;
        } else {
            filters.append( flt );
        }
    }
}
//******************************************************************************
void WorkThread::filterPortion(Ipp32fc *src, int len)
{
    auto it = filters.begin();

    while( it != filters.end() ) {
        // обработка порции данных
        (*it)->processData( src, len );

        // если при фильтрации произошла ошибка - удаляем такой обработчик и переходим к следующему
        if( (*it)->hasError() ) {
            delete (*it);
            filters.erase( it++ ); // it++ переходит к следующему и возвращает предыдущий, уже не нужный
        } else {
            ++it; // ничего не возвращаем, просто переходим к следующему
        }
    }
}
//******************************************************************************
void WorkThread::clearFilters()
{
    for( SimpleIppFIR2File *flt : filters ) {
        flt->stopProcessing();
        delete flt;
    }
    filters.clear();
}
//******************************************************************************
