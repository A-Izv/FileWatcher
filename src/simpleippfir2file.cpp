#include "simpleippfir2file.h"
#include "myfirtaps.h"
//------------------------------------------------------------------------------
SimpleIppFIR2File::SimpleIppFIR2File(const QString &dstFileName, float f , int maxInSizeSmpl) :
    error(false),
    filterInit(false),
    maxInLen(maxInSizeSmpl),
    rFreq(f)
{
    outFile = nullptr;
    dlyLine = nullptr;
    firBuff = nullptr;
    firSpec = nullptr;
    buf1    = nullptr;
    buf2    = nullptr;

    try {
        // выделяем память
        buf1    = ippsMalloc_32fc( maxInLen );
        buf2    = ippsMalloc_32fc( maxInLen );

        // создаем и открываем файл
        outFile = new QFile(dstFileName);
        if( !outFile->open(QIODevice::WriteOnly) ) throw("file open error");
        if( !outFile->resize(0) ) throw("file resize error");

        // подготовка децимирующего фильтра
        int specSize, buffSize;

        // 1 - запрос требуемого объема памяти
        CHK( ippsFIRMRGetSize32f_32fc( myFirTapsLen,
                                       1,               // upFactor
                                       DOWN_FACTOR,     // downFactor
                                       &specSize, &buffSize ) );

        // 2 - выделение памяти
        dlyLine = ippsMalloc_32fc( myFirTapsLen-1 );
        firBuff = ippsMalloc_8u( buffSize );
        firSpec = (IppsFIRSpec32f_32fc*)ippsMalloc_8u( specSize );

        // 3 - инициализация
        CHK( ippsFIRMRInit32f_32fc( myFirTaps,
                                    myFirTapsLen,
                                    1,          // upFactor
                                    0,          // upPhase
                                    DOWN_FACTOR,// downFactor
                                    0,          // downPhase
                                    firSpec ) );


        initFilter();
    }
    catch(...) {
        error = true;
    }
//    CHK( ippsFIRMR32f_32fc(const Ipp32fc* pSrc, Ipp32fc* pDst, int numIters, IppsFIRSpec32f_32fc* pSpec, const Ipp32fc* pDlySrc, Ipp32fc* pDlyDst, Ipp8u* pBuf)
}
//------------------------------------------------------------------------------
SimpleIppFIR2File::~SimpleIppFIR2File()
{
    ippFree( firSpec );
    ippFree( firBuff );
    ippFree( dlyLine );
    ippFree( buf2 );
    ippFree( buf1 );

    delete outFile;
}
//------------------------------------------------------------------------------
void SimpleIppFIR2File::initFilter()
{
    if( !error ) {
        try {
            CHK( ippsZero_32fc( dlyLine, myFirTapsLen-1) );
            interCount = 0;
            phase = 0;

            filterInit = true;
        }
        catch(...) {
            error = true;
        }
    }
}
//------------------------------------------------------------------------------
void SimpleIppFIR2File::processData(Ipp32fc *src, int len)
{
    error = error || ( len > maxInLen );
    if( !error && filterInit ) {
        try {
            int numIters, tmpI ;
            Ipp32fc *b;

            // сдвигаем реализацию на нулевую частоту
            CHK( ippsTone_32fc( buf1, len, 1, rFreq, &phase, ippAlgHintAccurate) );
            CHK( ippsMul_32fc_I( src, buf1, len) );

            if( interCount != 0 ) {
             // остался кусочек с прошлого раза
                tmpI = DOWN_FACTOR - interCount; // сколько отсчетов не хватает
                tmpI = qMin( tmpI, len ); // len может оказаться меньше tmpI (в конце входного файла)
                CHK( ippsCopy_32fc( buf1,       // добавляем данные в буфер с остаточком
                                    interBuf + interCount,
                                    tmpI) );
                interCount += tmpI;
                if( interCount == DOWN_FACTOR ) {
                 // набрали полный буфер
                    filterAndWrite( interBuf, 1 );
                    interCount = 0;
                }
                // смещаем указатель на входные данные - ведь отсчеты с начала уже использованы
                b = buf1 + tmpI;
                len -= tmpI;
            } else {
                b = buf1;
            }

            // фильтрация основной порции данных
            numIters = len/DOWN_FACTOR;
            if( numIters > 0 ) {
                filterAndWrite( b, numIters );
            }

            // проверяем, не остались ли необработанные отсчеты
            tmpI = len%DOWN_FACTOR;
            if( tmpI ) {
                CHK( ippsCopy_32fc( interBuf + interCount,   // в буфер с остатком переносим
                                    b + numIters*DOWN_FACTOR,// то, что оказалось не кратно итерации
                                    tmpI ) );
                interCount += tmpI;                 // запоминаем, сколько отсчетов сохранили
            }
        }
        catch(...) {
            error = true;
        }
    }
}
//------------------------------------------------------------------------------
void SimpleIppFIR2File::stopProcessing()
{
    if( !error && filterInit ) {
        try {
            int len, numIters;

            // получаем число итераций
            len         = myFirTapsLen-1;
            numIters    = len/DOWN_FACTOR;
            if( len%DOWN_FACTOR) numIters++;

            // формируем реализацию для "выталкивания" переходного процесса
            CHK( ippsZero_32fc( buf1, len) );
            filterAndWrite( buf1, numIters );

            // сбрасываем флаг, т.к. реализация отфильтрована
            filterInit = false;
        }
        catch(...) {
            error = true;
        }
    }
}
//------------------------------------------------------------------------------
void SimpleIppFIR2File::filterAndWrite(Ipp32fc *src, int numIters)
{// Вспомогательная функция для внутреннего употребления.
 // Фильтрует переданные данные и записывает результат в файл.
    int size;

    CHK( ippsFIRMR32f_32fc( src,
                            buf2,
                            numIters,
                            firSpec,
                            dlyLine,
                            dlyLine,
                            firBuff ) );

    numIters *= sizeof(Ipp32fc);// получаем число байт, которое хотим записать
    size = outFile->write( (char*)buf2, numIters ); // записываем
    if( size != numIters ) throw( "can't write data to file" ); // контролируем
}
//------------------------------------------------------------------------------
