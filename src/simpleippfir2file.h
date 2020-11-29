#ifndef SIMPLEIPPFIR2FILE_H
#define SIMPLEIPPFIR2FILE_H
//------------------------------------------------------------------------------
#include <QFile>
//------------------------------------------------------------------------------
// включаем генерацию исключения как реакцию на ошибку IPP
#ifndef IPP_CHK_WITH_EXCEPTIONS
    #define IPP_CHK_WITH_EXCEPTIONS
#endif
#include <ippCustom.h>
//------------------------------------------------------------------------------
class SimpleIppFIR2File
{
public:
            SimpleIppFIR2File( const QString &dstFileName,
                               float f,
                               int   maxInSizeSmpl );
           ~SimpleIppFIR2File();

    void    initFilter();
    void    processData( Ipp32fc *src, int len );
    void    stopProcessing();

    bool    hasError() {return error;}
private:
    static const int DOWN_FACTOR = 10;
private:
    bool                error;
    bool                filterInit;

    int                 maxInLen;

    QFile               *outFile;

    float               rFreq;
    float               phase;

    Ipp32fc             *dlyLine;
    Ipp8u               *firBuff;
    IppsFIRSpec32f_32fc *firSpec;   // ! новая возможность IPP!
                                    // Фильтрация комплексного сигнала действительным фильтром!

    Ipp32fc             *buf1;
    Ipp32fc             *buf2;

    Ipp32fc             interBuf[DOWN_FACTOR];
    int                 interCount;

    void                filterAndWrite( Ipp32fc *src, int numIters );
};
//------------------------------------------------------------------------------
#endif // SIMPLEIPPFIR2FILE_H
