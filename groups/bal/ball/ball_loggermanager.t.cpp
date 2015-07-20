// ball_loggermanager.t.cpp         -*-C++-*-

#include <ball_loggermanager.h>

#include <ball_defaultobserver.h>               // for testing only
#include <ball_fixedsizerecordbuffer.h>
#include <ball_loggermanagerconfiguration.h>
#include <ball_loggermanagerdefaults.h>
#include <ball_severity.h>
#include <ball_testobserver.h>                  // for testing only

#include <bdlqq_barrier.h>                      // for testing only
#include <bdlqq_xxxthread.h>                       // for testing only

#include <bslma_testallocator.h>                // for testing only

#include <bdlmxxx_schemaaggregateutil.h>           // for testing only

#include <bdlf_bind.h>                          // for testing only
#include <bdlf_placeholder.h>                   // for testing only

#include <bdlt_currenttime.h>                   // for testing only

#include <bslma_default.h>                      // for testing only
#include <bslma_defaultallocatorguard.h>        // for testing only
#include <bslma_managedptr.h>
#include <bslma_testallocator.h>                // for testing only
#include <bslma_testallocatorexception.h>       // for testing only

#include <bsls_log.h>
#include <bsls_platform.h>
#include <bsls_types.h>

#include <bslstl_stringref.h>

#include <bsl_iostream.h>
#include <bsl_fstream.h>
#include <bsl_map.h>
#include <bsl_new.h>         // placement 'new' syntax
#include <bsl_string.h>
#include <bsl_sstream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>

#include <bsl_cstdlib.h>     // atoi()
#include <bsl_cstring.h>     // strlen(), memset(), memcpy(), memcmp()
#include <bsl_c_ctype.h>     // tolower()

#include <bsl_c_stdio.h>     // tempnam()
#include <bsl_c_stdlib.h>    // rand_r()

#ifdef BSLS_PLATFORM_OS_UNIX
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

// Note: on Windows -> WinGDI.h:#define ERROR 0
#if defined(BSLS_PLATFORM_CMP_MSVC) && defined(ERROR)
#undef ERROR
#endif

using namespace BloombergLP;
using namespace bsl;  // automatically added by script
using namespace bdlf::PlaceHolders;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//                              Overview
//                              --------
// TBD doc
//
//.............................................................................
//                           **  N  O  T  E **
//
// The deprecated 'initSingleton' method has been removed from the test cases
// **EXCEPT** for the 16 cases that test 'initSingleton', which must remain
// until the methods are actually *removed*.
//
// When those methods are removed, it is STILL NECESSARY to keep those 16
// test cases, changing them to the equivalent cases setting the
// 'ball::LoggerManagerConfiguration' object and instantiating the logger
// manager directly.
//
// This is the case because the 'initSingleton' methods are now implemented
// using the configuration object and the constructor, so those features are
// currently tested through the 'initSingleton' imp.  When that's removed,
// THE TEST MUST REMAIN!
//.............................................................................
//
//-----------------------------------------------------------------------------
// 'ball::Logger' private interface (tested indirectly):
// [14] ball::Logger(Obs*, *buffer, Sch*, Pop&, Pac&, *ba);
// [14] ~ball::Logger();
// [14] void publish(ball::Transmission::Cause cause);
//
// 'ball::Logger' public interface:
// [14] void logMessage(category, severity, file, line, msg);
// [29] bool logMessage(const ball::Category *, int , ball::Record *);
// [14] void publish();
// [14] void removeAll();
// [14] char *messageBuffer();
// [14] int messageBufferSize() const;
// [32] int numRecordsInUse() const;
//
// 'ball::LoggerManager' private interface (tested indirectly):
// [14] void publishAllImp(ball::Transmission::Cause cause);
//
// 'ball::LoggerManager' public interface:
// [ 2] static void initSingleton(Obs*, *ba = 0);
// [ 3] static void initSingleton(Obs*, Fdt&, *ba = 0);
// [ 4] static void initSingleton(Obs*, Dtc&, *ba = 0);
// [ 5] static void initSingleton(Obs*, Dtc&, Fdt&, *ba = 0);
// [ 6] static void initSingleton(Obs*, Sch&, Pop&, *ba = 0);
// [ 7] static void initSingleton(Obs*, Fdt&, Sch&, Pop&, *ba = 0);
// [ 8] static void initSingleton(Obs*, Dtc&, Sch&, Pop&, *ba = 0);
// [ 9] static void initSingleton(Obs*, Dtc&, Fdt&, Sch&, Pop&, *ba = 0);
// [10] static bool isInitialized();
// [ 2] static ball::LoggerManager& singleton();
// [24] static ball::Record *getRecord(const char *file, int line);
// [24] static void logMessage(int severity, ball::Record *record);
// [10] ball::LoggerManager(Obs*, const ball::LoggerManagerConfiguration&,*ba=0);
// [10] ~ball::LoggerManager();
// [10] static void initSingleton(Obs*, const Configuration&);
// [16] ball::Logger *allocateLogger(ball::RecordBuffer *buffer)
// [16] ball::Logger *allocateLogger(ball::RecordBuffer *buffer, int msgBufSize);
// [16] void deallocateLogger(ball::Logger *logger);
// [16] void setLogger(ball::Logger *logger);
// [14] ball::Logger& getLogger();
// [11] Cat *lookupCategory(const char *name);
// [11] Cat *addCategory(const char *name, int, int, int, int);
// [11] const Cat *setCategory(const char *name);
// [11] Cat *setCategory(const char *name, int, int, int, int);
// [  ] Cat& defaultCategory();
// [11] int setDefaultThresholdLevels(int, int, int, int);
// [11] int resetDefaultThresholdLevels();
// [11] void setCategoryThresholdsToCurrentDefaults(Cat *cat);
// [11] void setCategoryThresholdsToFactoryDefaults(Cat *cat);
// [ 4] void setDefaultThresholdLevelsCallback(Dtc *cb);
// [11] void setMaxNumCategories(int);
// [14] void publishAll();
// [11] const Cat *lookupCategory(const char *name) const;
// [11] const Cat& defaultCategory() const;
// [ 2] const Obs *observer() const;
// [  ] const Pop *populator() const;
// [11] int defaultRecordThresholdLevel() const;
// [11] int defaultPassThresholdLevel() const;
// [11] int defaultTriggerThresholdLevel() const;
// [11] int defaultTriggerAllThresholdLevel() const;
// [11] int maxNumCategories() const;
// [  ] int numCategories() const;
// [28] bool isCategoryEnabled(const ball::Category *, int) const;
//
// 'ball::LoggerManagerScopedGuard' public interface:
// [25] ball::LoggerManagerScopedGuard(Obs*, const Configuration&, ba = 0);
// [25] ~ball::LoggerManagerScopedGuard();
//
// 'ball::LoggerCategoryIter' public interface:
// [12] ball::LoggerCategoryIter(const ball::LoggerManager& lm);
// [12] ~ball::LoggerCategoryIter();
// [12] void operator++();
// [12] operator const void *() const;
// [12] const Cat& operator()() const;
//
// 'ball::LoggerCategoryManip' public interface:
// [13] ball::LoggerCategoryManip(ball::LoggerManager *lm);
// [13] ~ball::LoggerCategoryManip();
// [13] void advance();
// [13] Cat& operator()();
// [13] operator const void *() const;
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [26] LOW-LEVEL LOGGING
// [17] PERFORMANCE TEST
// [18] TESTING CONCURRENT ACCESS TO 'd_loggers'
// [19] TESTING CONCURRENT ACCESS TO 'd_defaultLoggers'
// [20] TESTING CONCURRENT ACCESS TO 'ball::LoggerManager::setCategory'
// [21] TESTING CONCURRENT ACCESS TO primary 'initSingleton'
// [22] TESTING CONCURRENT ACCESS TO 'ball::LoggerManager::lookupCategory'
// [33] USAGE EXAMPLE #1
// [34] USAGE EXAMPLE #2
// [35] USAGE EXAMPLE #4
//=============================================================================
//                      STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
static int testStatus = 0;

void aSsErT(int c, const char *s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (0 <= testStatus && testStatus <= 100) ++testStatus;
    }
}

#define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
   if (!(X)) { cout << #I << ": " << I << "\n"; aSsErT(1, #X, __LINE__); }}

#define LOOP2_ASSERT(I,J,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " \
              << J << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" \
              << #K << ": " << K << "\n"; aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) cout << #X " = " << (X) << endl; // Print identifier and value.
#define Q(X) cout << "<| " #X " |>" << endl;  // Quote identifier literally.
#define P_(X) cout << #X " = " << (X) << ", "<< flush; // P(X) without '\n'
#define F_ __FILE__                           // current source file name
#define L_ __LINE__                           // current Line number
#define T_()  cout << "\t" << flush;          // Print tab w/o newline

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

typedef ball::LoggerManager              Obj;

typedef ball::Logger                     Logger;

typedef ball::Category                   Cat;
typedef ball::Observer                   Obs;
typedef ball::Record                     Rec;
typedef ball::RecordAttributes           Attr;
typedef ball::RecordBuffer               RecBuf;

typedef bdlmxxx::List                       List;
typedef bdlmxxx::Schema                     Sch;

typedef ball::ThresholdAggregate             Thresholds;
typedef Logger::PublishAllTriggerCallback   Pac;
typedef Logger::UserPopulatorCallback       Pop;

typedef Obj::CategoryNameFilterCallback     Cnf;
typedef Obj::DefaultThresholdLevelsCallback Dtc;
typedef Obj::FactoryDefaultThresholds       Fdt;

const int NUM_LOGGERS = 10;
const int BUF_SIZE    = 32768;

// Note: when this buffer is declared local to 'main', an "unknown software
// exception" is triggered on Windows (presumably due to the buffer's size).
// *** no more needed
// char *globalBuffer[NUM_LOGGERS][BUF_SIZE];

const char *NAMES[] = {  // test category names
    "A",
    "AB",
    "ab",
    "ABC",
    "ABCDEDFGHIJKL",
    "A.B.C.E",
    "abcd_efgh_.abcd",
    "a.b.c.e.d.f",
    "1",
    "12345",
    "123abc456def",
    "A Really really really looooooooooooooooooooooooooooooooooooooong name",
};
const int NUM_NAMES = sizeof NAMES / sizeof *NAMES;

struct my_Severity {
    enum Level {
        BAEL_FATAL =  32,
        BAEL_ERROR =  64,
        BAEL_WARN  =  96,
        BAEL_INFO  = 128,
        BAEL_DEBUG = 160,
        BAEL_TRACE = 192
    };
};

// *** Keep the following in sync with the .cpp file. ***

const int   MESSAGE_BUFFER_SIZE   = 8192;
const char *DEFAULT_CATEGORY_NAME = "";

const unsigned char FACTORY_RECORD     =  0;
const unsigned char FACTORY_PASS       = 64;
const unsigned char FACTORY_TRIGGER    =  0;
const unsigned char FACTORY_TRIGGERALL =  0;

void executeInParallel(int numThreads, bdlqq::ThreadUtil::ThreadFunction func)
   // Create the specified 'numThreads', each executing the specified 'func'.
   // Number each thread (sequentially from 0 to 'numThreads-1') by passing i
   // to i'th thread.  Finally join all the threads.
{
    bdlqq::ThreadUtil::Handle *threads =
                               new bdlqq::ThreadUtil::Handle[numThreads];
    ASSERT(threads);

    for (int i = 0; i < numThreads; ++i) {
        bdlqq::ThreadUtil::create(&threads[i], func, (void*)i);
    }
    for (int i = 0; i < numThreads; ++i) {
        bdlqq::ThreadUtil::join(threads[i]);
    }

    delete [] threads;
}

//=============================================================================
//                         CASE 32 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_32 {
class Information {
  private:
    bsl::string d_heading;
    bsl::string d_contents;

  public:
    Information(const char *heading, const char *contents);
    const bsl::string& heading() const;
    const bsl::string& contents() const;
};

Information::Information(const char *heading, const char *contents)
: d_heading(heading)
, d_contents(contents)
{
}

const bsl::string& Information::heading() const
{
    return d_heading;
}

const bsl::string& Information::contents() const
{
    return d_contents;
}

bsl::ostream& operator<<(bsl::ostream& stream,
                         const Information& information)
{
    stream << information.heading() << endl;
    stream << '\t';
    stream << information.contents() << endl;
    return stream;
}

void logInformation(ball::Logger *logger,
                    const Information& information,
                    ball::Severity::Level severity,
                    const ball::Category& category,
                    const char* fileName,
                    int lineNumber)
{
    ball::Record *record = logger->getRecord(fileName,lineNumber);
    ball::RecordAttributes& attributes = record->fixedFields();

    attributes.setFileName(fileName);
    attributes.setLineNumber(lineNumber);

    ostream os(&attributes.messageStreamBuf());
    os << information;
    logger->logMessage(category, severity, record);
}
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_32

// ============================================================================
//                         CASE 29 RELATED ENTITIES
// ----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_29 {
class TestDestroyObserver : public ball::Observer {
    int d_releaseCnt;
public:
    explicit TestDestroyObserver() : d_releaseCnt(0)
    {
    }

    virtual void releaseRecords()
    {
        d_releaseCnt++;
    }
    int getReleaseCnt()
    {
        return d_releaseCnt;
    }
};
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_29

//=============================================================================
//                         CASE 22 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_22 {
enum {
    NUM_THREADS = 4 // number of threads
};
ball::LoggerManager *manager;

const char *LC[] = {
    "ball_loggermanagerconfiguration",
    "ball_multiplexobserver",
    "ball_recordstringformatter",
    "ball_thresholdaggregate"
};

const char *UC[] = {
    "BALL_LOGGERMANAGERCONFIGURATION",
    "BALL_MULTIPLEXOBSERVER",
    "BALL_RECORDSTRINGFORMATTER",
    "BALL_THRESHOLDAGGREGATE"
};

struct ThreadData {
    int index;  // index of current thread
};

bdlqq::Barrier barrier(NUM_THREADS);

extern "C" {
    void *workerThread29(void *arg)
    {
        ThreadData *p = (ThreadData*)arg;
        ASSERT(manager->lookupCategory(UC[p->index]) != 0);
        return 0;
    }
}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_22

//=============================================================================
//                         CASE 21 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_21 {
enum {
    NUM_THREADS = 4 // number of threads
};

struct ThreadData {
    int                   index;    // the index of current thread

    ball::DefaultObserver *observer; // the observer provided to current thread

    ball::LoggerManager   *manager;  // the instance observed by current thread
};

bdlqq::Barrier barrier(NUM_THREADS);

extern "C" {
    void *workerThread28(void *arg)
    {
        ThreadData *p = (ThreadData*)arg;

        ball::LoggerManagerConfiguration configuration;
        configuration.setDefaultThresholdLevelsIfValid(p->index,
                                                       p->index,
                                                       p->index,
                                                       p->index);

        barrier.wait();
        ball::LoggerManager::initSingleton(p->observer, configuration);
        p->manager = &ball::LoggerManager::singleton();

        return 0;
    }
}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_21

//=============================================================================
//                         CASE 20 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_20 {
enum {
    NUM_THREADS = 4 // number of threads
};
ball::LoggerManager *manager;

extern "C" {
    void *workerThread27(void *)
    {
        ASSERT(manager->setCategory("TEST-CATEGORY") != 0);
        return 0;
    }
}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_20

//=============================================================================
//                         CASE 19 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_19 {
enum {
    NUM_THREADS = 2,        // number of threads
    NUM_ITERATIONS = 1000   // number of iterations
};
ball::LoggerManager *manager;
enum { MAX_LIMIT = 32 * 1024 };
ball::FixedSizeRecordBuffer buf(MAX_LIMIT);

bdlqq::Barrier barrier(NUM_THREADS);
extern "C" {
    void *workerThread26(void *arg)
    {
        int remainder = (int)(bsls::Types::IntPtr)arg % 2;
        if (remainder == 1) {  // odd numbered threads
            ball::Logger *logger = manager->allocateLogger(&buf);
            barrier.wait();
            for (int i=0; i<NUM_ITERATIONS; ++i) {
                manager->setLogger(logger);
                ASSERT(logger == &manager->getLogger());
            }
            manager->deallocateLogger(logger);
        }
        else {                 // odd numbered threads
            const ball::Logger *logger = &manager->getLogger();
            barrier.wait();
            for (int i=0; i<NUM_ITERATIONS; ++i) {
                ASSERT(logger == &manager->getLogger());
            }
        }

        return 0;
    }
}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_19

//=============================================================================
//                         CASE 18 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_18 {
enum {
    NUM_THREADS = 3,        // number of threads
    NUM_ITERATIONS = 10000  // number of iterations
};

class my_publishCountingObserver : public ball::Observer {
    // This concrete implementation of 'ball::Observer' maintains a count
    // of the number of messages published to it and gives access to that
    // count through 'publishCount'.
    bsls::AtomicInt d_publishCount;
  public:
    // CONSTRUCTORS
    my_publishCountingObserver() : d_publishCount(0)
    {
    }

    ~my_publishCountingObserver()
    {
    }

    //MANIPULATORS
    void publish(const ball::Record& , const ball::Context& )
    {
        ++d_publishCount;
    }

    //ACCESSORS
    int publishCount() const
    {
        return d_publishCount;
    }
};

ball::Category *cat;
ball::LoggerManager *manager;
enum { MAX_LIMIT = 32 * 1024 };
bdlqq::Barrier barrier(NUM_THREADS);
extern "C" {
    void *workerThread25(void *)
    {
        barrier.wait();
        for (int i=0; i<NUM_ITERATIONS; ++i) {
            ball::FixedSizeRecordBuffer buf(MAX_LIMIT);
            ball::Logger *logger = manager->allocateLogger(&buf);
            logger->logMessage(*cat,
                               ball::Severity::BAEL_ERROR,
                               __FILE__,
                               __LINE__,
                               "test-message");
            manager->deallocateLogger(logger);
        }
        return 0;
    }

}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_18

//=============================================================================
//                         CASE 15 RELATED ENTITIES
//-----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_15 {

class MyObserver : public ball::Observer {
    // This concrete implementation of 'ball::Observer' maintains a count
    // of the number of messages published to it and gives access to that
    // count through 'publishCount'.

    int                 d_publishCount;
    bsl::ostream&       d_stream;

  public:
    // CREATORS
    MyObserver(bsl::ostream& stream) : d_publishCount(0), d_stream(stream)
    {
    }

    // MANIPULATORS
    void publish(const ball::Record& record, const ball::Context& context)
    {
        ++d_publishCount;
        d_stream << "Log " << context.recordIndex() + 1
                 << " of " << context.sequenceLength()
                 << " : "  << record.fixedFields().message()
                 << "\n"   << bsl::flush;
    }

    // ACCESSORS
    int publishCount() const
    {
        return d_publishCount;
    }
};

}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_15

//=============================================================================
//                               USAGE EXAMPLE
//-----------------------------------------------------------------------------

static
int factorial(int n)
    // Return the factorial of the specified value 'n' if n! can be
    // represented as an 'int', and a negative value otherwise.
{
    static ball::Logger& logger = ball::LoggerManager::singleton().getLogger();
    static const ball::Category *factorialCategory =
                         ball::LoggerManager::singleton().setCategory(
                                           "EQUITIES.GRAPHICS.MATH.FACTORIAL");
    if (0 > n) {
        logger.logMessage(*factorialCategory,
                          my_Severity::BAEL_WARN,
                          __FILE__,
                          __LINE__,
                          "Attempt to take factorial of negative value.");
        return n;
    }

    // maximum value accepted by 'factorial'
    enum { MAX_ARGUMENT = 13 };

    if (MAX_ARGUMENT < n) {
        logger.logMessage(*factorialCategory,
                          my_Severity::BAEL_WARN,
                          __FILE__,
                          __LINE__,
                          "Result too large for 'int'.");
        return -n;
    }
    int product = 1;
    while (1 < n) {
       product *= n;
       --n;
    }

    logger.logMessage(*factorialCategory,
                      my_Severity::BAEL_TRACE,
                      __FILE__,
                      __LINE__,
                      "Exiting 'factorial' successfully.");

    return product;
}

static
int getDefaultThresholdLevels(int                       *recordLevel,
                              int                       *passLevel,
                              int                       *triggerLevel,
                              int                       *triggerAllLevel,
                              char                       delimiter,
                              const ball::LoggerManager&  loggerManager,
                              const char                *categoryName)
{
    ASSERT(recordLevel);
    ASSERT(passLevel);
    ASSERT(triggerLevel);
    ASSERT(triggerAllLevel);
    ASSERT(categoryName);

    enum { SUCCESS = 0, FAILURE = -1 };

    bsl::string buffer(categoryName);

    while (1) {
        for (ball::LoggerCategoryIter it(loggerManager); it; ++it) {
            const ball::Category& category = it();
            const char *name = category.categoryName();
            if (0 == bsl::strcmp(name, buffer.c_str())) {
                *recordLevel     = category.recordLevel();
                *passLevel       = category.passLevel();
                *triggerLevel    = category.triggerLevel();
                *triggerAllLevel = category.triggerAllLevel();
                return SUCCESS;                                       // RETURN
            }
        }
        char *p = bsl::strrchr(const_cast<char *>(buffer.c_str()), delimiter);
        if (!p) {
            return FAILURE;                                           // RETURN
        }
        *p = 0;
    }
}

static Obj* g_overrideManager;

static
void inheritThresholdLevelsUsingManager(int        *recordLevel,
                                        int        *passLevel,
                                        int        *triggerLevel,
                                        int        *triggerAllLevel,
                                        const char *categoryName,
                                        const ball::LoggerManager *lm)
{
    ASSERT(recordLevel);
    ASSERT(passLevel);
    ASSERT(triggerLevel);
    ASSERT(triggerAllLevel);
    ASSERT(categoryName);

    if (0 == lm) {
        lm = g_overrideManager;
    }

    if (0 != getDefaultThresholdLevels(recordLevel,
                                       passLevel,
                                       triggerLevel,
                                       triggerAllLevel,
                                       '.',
                                       *lm,
                                       categoryName)) {
        *recordLevel     = lm->defaultRecordThresholdLevel();
        *passLevel       = lm->defaultPassThresholdLevel();
        *triggerLevel    = lm->defaultTriggerThresholdLevel();
        *triggerAllLevel = lm->defaultTriggerAllThresholdLevel();
    }
}

static
void inheritThresholdLevels(int        *recordLevel,
                            int        *passLevel,
                            int        *triggerLevel,
                            int        *triggerAllLevel,
                            const char *categoryName)
{
    const ball::LoggerManager& lm = ball::LoggerManager::singleton();
    inheritThresholdLevelsUsingManager(recordLevel, passLevel,
                                       triggerLevel, triggerAllLevel,
                                       categoryName, &lm);
}

static int globalFactorialArgument;  // TBD kludge

static
void myPopulator(bdlmxxx::List *list, const bdlmxxx::Schema& schema)
{
    const bdlmxxx::RecordDef *rec = schema.lookupRecord("FACTORIAL");
    ASSERT(rec);
    bdlmxxx::SchemaAggregateUtil::initListShallow(list, *rec);
    list->theModifiableInt(0) = globalFactorialArgument;
}

// ============================================================================
//                         CASE 30 RELATED ENTITIES
// ----------------------------------------------------------------------------
namespace BAEL_LOGGERMANAGER_TEST_CASE_30 {
enum {
    NUM_THREADS = 4 // number of threads
};
ball::LoggerManager *manager;
bdlf::Function<void (*)(int *, int *, int *, int *, const char *)> function;
bdlqq::Barrier barrier(NUM_THREADS + 1);
bdlqq::Condition condition;

void callback(int *r, int *p, int *t, int *a, const char *c)
    // Wait for all the setting threads to be ready to set the callback, invoke
    // the 'inheritThresholdLevels' method on the specified 'r', 'p', 't', 'a',
    // and 'c', and then time out waiting for the setting threads to finish.
    // (They can't, because a lock is held until this function returns.)
{
    barrier.wait();
    inheritThresholdLevels(r, p, t, a, c);
    bdlqq::Mutex m;
    bdlqq::LockGuard<bdlqq::Mutex> guard(&m);
    ASSERT(-1 == condition.timedWait(&m, bdlt::CurrentTime::now() + 3));
}

void *setCategory(void *)
    // Call 'setCategory' in order to invoke the 'callback' method above.
{
    manager->setCategory("joe");
    return 0;
}

extern "C" {
    void *workerThread30(void *)
        // Wait for all threads to be ready, then attempt to set the callback
        // and signal completion.  The thread waiting for the signal will time
        // out, because the setter mutex is locked by that thread.
    {
        barrier.wait();
        manager->setDefaultThresholdLevelsCallback(&function);
        condition.signal();
        return 0;
    }
}  // extern "C"
}  // close namespace BAEL_LOGGERMANAGER_TEST_CASE_30

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

static
void toLower(bsl::string *buffer, const char *s)
{
    ASSERT(buffer);
    ASSERT(s);

    buffer->clear();
    while (*s) {
        buffer->push_back(static_cast<char>(tolower(*s)));
        ++s;
    }
    buffer->push_back(0);
}

static
void simpleThresholdLevels(int        *recordLevel,
                           int        *passLevel,
                           int        *triggerLevel,
                           int        *triggerAllLevel,
                           const char *)
{
    *recordLevel     = 44;
    *passLevel       = 33;
    *triggerLevel    = 22;
    *triggerAllLevel = 11;
}

static
void simplePopulator(bdlmxxx::List *list, const bdlmxxx::Schema& schema)
{
    const bdlmxxx::RecordDef *rec = schema.lookupRecord("initSingleton");
    ASSERT(rec);
    bdlmxxx::SchemaAggregateUtil::initListShallow(list, *rec);
    list->theModifiableInt(0)    = 1066;
    list->theModifiableString(1) = "XXX";
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
    int veryVerbose = argc > 3;
    int veryVeryVerbose = argc > 4;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;;

    switch (test) { case 0:  // Zero is always the leading case.
      case 35: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE #4
        //
        // Concerns:
        //   The usage example provided in the component header file must
        //   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE #4
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing Usage Example 4" << endl
                                  << "=======================" << endl;

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_32;

        ball::DefaultObserver observer(cout);
        ball::LoggerManagerConfiguration configuration;
        ball::LoggerManagerScopedGuard guard(&observer, configuration);
        ball::LoggerManager& mLM = ball::LoggerManager::singleton();
        ball::Category *cat = mLM.addCategory("test-category",
                                             ball::Severity::BAEL_INFO,
                                             ball::Severity::BAEL_WARN,
                                             ball::Severity::BAEL_ERROR,
                                             ball::Severity::BAEL_FATAL);

        ball::Logger& logger = mLM.getLogger();

        Information information("MY-HEADING", "MY-CONTENTS");

        if (verbose) {
            logInformation(&logger,
                           information,
                           ball::Severity::BAEL_WARN,
                           *cat,
                           __FILE__,
                           __LINE__);
        }

      } break;
      case 34: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE #2
        //
        // Concerns:
        //   The usage example provided in the component header file must
        //   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE #2
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing Usage Example 2" << endl
                                  << "=======================" << endl;

        ball::DefaultObserver observer(cout);

        ball::LoggerManager::CategoryNameFilterCallback nameFilter(&toLower);

        ball::LoggerManager::DefaultThresholdLevelsCallback
            thresholdsCallback(&inheritThresholdLevels);

        int recordLevel     = 125;
        int passLevel       = 100;
        int triggerLevel    =  75;
        int triggerAllLevel =  50;

        ball::LoggerManagerDefaults defaults;
        defaults.setDefaultThresholdLevelsIfValid(recordLevel,
                                                  passLevel,
                                                  triggerLevel,
                                                  triggerAllLevel);

        ball::LoggerManagerConfiguration configuration;
        configuration.setDefaultValues(defaults);
        configuration.setCategoryNameFilterCallback(nameFilter);
        configuration.setDefaultThresholdLevelsCallback(thresholdsCallback);

        ball::LoggerManagerScopedGuard guard(&observer, configuration);

        ball::LoggerManager& loggerManager = ball::LoggerManager::singleton();

        const ball::Category& defaultCat = loggerManager.defaultCategory();
        ASSERT(125 == defaultCat.recordLevel());
        ASSERT(100 == defaultCat.passLevel());
        ASSERT( 75 == defaultCat.triggerLevel());
        ASSERT( 50 == defaultCat.triggerAllLevel());

        const ball::Category *cat1 =
                     loggerManager.addCategory("BloombergLP", 128, 96, 64, 32);
        ASSERT(cat1 == loggerManager.lookupCategory("BLOOMBERGLP"));
        ASSERT(   0 == bsl::strcmp("bloomberglp", cat1->categoryName()));
        ASSERT( 128 == cat1->recordLevel());
        ASSERT(  96 == cat1->passLevel());
        ASSERT(  64 == cat1->triggerLevel());
        ASSERT(  32 == cat1->triggerAllLevel());

        const ball::Category *cat2 =
                             loggerManager.setCategory("BloombergLP.bae.bael");
        ASSERT(cat2 == loggerManager.lookupCategory("bloomberglp.bae.bael"));
        ASSERT(   0 == bsl::strcmp("bloomberglp.bae.bael",
                                   cat2->categoryName()));
        ASSERT( 128 == cat2->recordLevel());
        ASSERT(  96 == cat2->passLevel());
        ASSERT(  64 == cat2->triggerLevel());
        ASSERT(  32 == cat2->triggerAllLevel());

        const ball::Category *cat3 =
                               loggerManager.setCategory("Bloomberg.equities");
        ASSERT(cat3 == loggerManager.lookupCategory("BLOOMBERG.EQUITIES"));
        ASSERT(   0 == bsl::strcmp("bloomberg.equities",
                                   cat3->categoryName()));
        ASSERT( 125 == cat3->recordLevel());
        ASSERT( 100 == cat3->passLevel());
        ASSERT(  75 == cat3->triggerLevel());
        ASSERT(  50 == cat3->triggerAllLevel());

      } break;
      case 33: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE #1
        //
        // Concerns:
        //   The usage example provided in the component header file must
        //   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE #1
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing Usage Example 1" << endl
                                  << "=======================" << endl;

        ball::TestObserver observer(cout);

        ball::LoggerManagerConfiguration configuration;
        configuration.setDefaultThresholdLevelsIfValid(
                                                   ball::Severity::BAEL_WARN);


        ball::LoggerManagerScopedGuard guard(&observer, configuration);

        ball::LoggerManager::singleton().addCategory("equities",
                                                    my_Severity::BAEL_DEBUG,
                                                    my_Severity::BAEL_WARN,
                                                    my_Severity::BAEL_ERROR,
                                                    my_Severity::BAEL_FATAL);

        if (veryVerbose) observer.setVerbose(1);
        globalFactorialArgument = 0;
        factorial(0);
        if (veryVerbose && observer.numPublishedRecords()) {
            cout << observer.lastPublishedRecord() << endl;
        }

        globalFactorialArgument = -1;
        factorial(-1);
        if (veryVerbose && observer.numPublishedRecords()) {
            cout << observer.lastPublishedRecord() << endl;
        }

        globalFactorialArgument = 10;
        factorial(10);
        if (veryVerbose && observer.numPublishedRecords()) {
            cout << observer.lastPublishedRecord() << endl;
        }

        globalFactorialArgument = 20;
        factorial(20);
        if (veryVerbose && observer.numPublishedRecords()) {
            cout << observer.lastPublishedRecord() << endl;
        }

      } break;
      case 32: {
        // --------------------------------------------------------------------
        // TESTING: 'ball::Logger::numRecordsInUse'
        //   Ensure that the method correctly returns the number of
        //   outstanding messages.
        //
        // Concerns:
        //: 1 The method returns 0 if no records have been obtained through a
        //:   call to 'getRecord'.
        //:
        //: 2 The number of records reproted increases by 1 for every call to
        //:   'getRecord', and decreases by 1 for every call to 'logRecord'.
        //:
        //
        // Plan:
        //: 1 Obtain several records using 'getRecord', calling
        //:   'numRecordsInUse' to verify the number of records increases as
        //:   expected, then call 'logMessage' and verify that
        //:   'numRecordsInUse' decreases as expected.
        //
        // Testing:
        //   int numRecordsInUse() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl
                          << "TESTING: 'ball::Logger::numRecordsInUse'" << endl
                          << "=======================================" << endl;


        bslma::TestAllocator ta(veryVeryVerbose);
        ball::TestObserver to(bsl::cout, &ta);
        ball::LoggerManagerConfiguration lmc;
        ball::LoggerManager::initSingleton(&to, lmc, &ta);
        ball::LoggerManager& manager = ball::LoggerManager::singleton();

        ball::Category& category = manager.defaultCategory();

        if (verbose) cout << "Testing obtaining and returning several records."
                          << endl;
        {
            ASSERT(0 == manager.getLogger().numRecordsInUse());

            ball::Record *r1 = manager.getLogger().getRecord("X", 1);

            ASSERT(1 == manager.getLogger().numRecordsInUse());

            ball::Record *r2 = manager.getLogger().getRecord("X", 1);

            ASSERT(2 == manager.getLogger().numRecordsInUse());

            ball::Record *r3 = manager.getLogger().getRecord("X", 1);

            ASSERT(3 == manager.getLogger().numRecordsInUse());

            manager.getLogger().logMessage(category, 1, r1);

            ASSERT(2 == manager.getLogger().numRecordsInUse());

            manager.getLogger().logMessage(category, 1, r2);

            ASSERT(1 == manager.getLogger().numRecordsInUse());

            manager.getLogger().logMessage(category, 1, r3);

            ASSERT(0 == manager.getLogger().numRecordsInUse());
        }


        ball::LoggerManager::shutDownSingleton();

      } break;
      case 31: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO
        // 'ball::LoggerManager::setDefaultThresholdLevelsCallback':
        //   Verify Concurrent access to 'setDefaultThresholdLevelsCallback'.
        //
        // Concerns:
        // That multiple threads can safely invoke
        // 'setDefaultThresholdLevelsCallback' with the same callback argument.
        //
        // Plan:
        // Create several threads, each of which invoke
        // 'setDefaultThresholdLevelsCallback' with the same callback argument.
        //
        // Testing:
        //   void setDefaultThresholdLevelsCallback(Dtc *cb);
        // --------------------------------------------------------------------
        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO "
                 << "'ball::LoggerManager::setDefaultThresholdLevelsCallback'"
                 << endl
                 << "============================="
                 << "======================================================="
                 << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_30;

        ball::DefaultObserver observer(cout);
        ball::LoggerManagerConfiguration configuration;
        ball::LoggerManagerScopedGuard guard(&observer, configuration);
        manager = &Obj::singleton();
        function = &callback;
        manager->setDefaultThresholdLevelsCallback(&function);
        bdlqq::ThreadUtil::Handle handle;
        bdlqq::ThreadUtil::create(&handle, setCategory, 0);
        executeInParallel(NUM_THREADS, workerThread30);
        bdlqq::ThreadUtil::join(handle);
      } break;
      case 30: {
        // --------------------------------------------------------------------
        // TESTING: 'ball::LoggerManager' calls 'ball::Observer::releaseRecords'
        //          on destruction.
        //
        // Concerns:
        //   We want to demonstrate that a logger manager correctly invokes
        //   'releaseRecords' of its observer when the logger manager is being
        //   destroyed.
        //
        // Plan:
        //   Create a 'TestDestroyObserver' which has a count of its
        //   'releaseRecords' called.  Install the observer to a logger manager
        //   object with limited life time.  When the logger manager gets
        //   destroyed, check that the count in the observer increases by one.
        //
        // Testing:
        //   This test exercises '~ball::LoggerManager()', but does not test
        //   that function.
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "Testing 'releaseRecords' calling on destruction" << endl
                 << "===============================================" << endl;

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_29;
        TestDestroyObserver testObserver;

        ASSERT(0 == testObserver.getReleaseCnt());
        {
            ball::LoggerManagerConfiguration mLMC;
            ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
            Obj::singleton();
        }
        ASSERT(1 == testObserver.getReleaseCnt());
      } break;
      case 29: {
        // --------------------------------------------------------------------
        // TESTING: 'ball::Logger::logMessage' (RULE BASED LOGGING)
        //
        // Concerns:
        //: 1 Verify that 'logMessage' uses the currently installed rules when
        //:   determining whether to publish a message.
        //:
        //: 2 Verify that 'logMessage' releases memory, both for records that
        //:   are published, and records that are not published.
        //
        // Plan:
        //: 1 Create a series of test threshold values and a series of
        //:   threshold-aggregate values.  Maintain, over the test, the
        //:   expected number of published records and the number of records
        //:   currently in the buffer, at any time (note that the records in
        //:   the buffered are added to the published records when a trigger
        //:   occurs).  For each threshold-aggregate value:   create a category
        //:   with that threshold-aggregate value; test the 'logMessage' method
        //:   with a variety of severity levels without a logging rule, then
        //:   for each test threshold-aggregate value, create a logging rule
        //:   that applies to the category and test the 'logMessage' method.
        //:   Compare the expected number of published records against the
        //:   actual number of published records.
        //:
        //: 2 Call 'logMessage' with severities that will, and will not,
        //:   cause a record to be published.  Use 'numRecordsInUse' to verify
        //:   the logs are released back to the pool.
        //
        // Testing:
        //   bool logMessage(const ball::Category *category,
        //                   int                  severity,
        //                   ball::Record         *record);
        // --------------------------------------------------------------------

        if (verbose)
            bsl::cout << bsl::endl
                      << "Testing Rule Based Logging: logMessage\n"
                      << "======================================\n";
        using namespace BloombergLP;

        int VALUES[] = { 1,
                         ball::Severity::BAEL_FATAL - 1,
                         ball::Severity::BAEL_FATAL,
                         ball::Severity::BAEL_FATAL + 1,
                         ball::Severity::BAEL_ERROR - 1,
                         ball::Severity::BAEL_ERROR,
                         ball::Severity::BAEL_ERROR + 1,
                         ball::Severity::BAEL_WARN - 1,
                         ball::Severity::BAEL_WARN,
                         ball::Severity::BAEL_WARN + 1,
                         ball::Severity::BAEL_INFO - 1,
                         ball::Severity::BAEL_INFO,
                         ball::Severity::BAEL_INFO + 1,
                         ball::Severity::BAEL_DEBUG - 1,
                         ball::Severity::BAEL_DEBUG,
                         ball::Severity::BAEL_DEBUG + 1,
                         ball::Severity::BAEL_TRACE - 1,
                         ball::Severity::BAEL_TRACE,
                         ball::Severity::BAEL_TRACE + 1
        };

        const int NUM_VALUES = sizeof (VALUES)/sizeof(*VALUES);
        bsl::vector<Thresholds> thresholds;
        for (int i = 0; i < NUM_VALUES; ++i) {
            thresholds.push_back(Thresholds(VALUES[i], 255, 255, 255));
            thresholds.push_back(Thresholds(255, VALUES[i], 255, 255));
            thresholds.push_back(Thresholds(255, 255, VALUES[i], 255));
            thresholds.push_back(Thresholds(255, 255, 255, VALUES[i]));
        }

        // We must keep track of the total number of published records, and
        // the number of buffered records.  The accumulated buffered records
        // will be published when logged message is at a higher severity than
        // the trigger level.
        int numPublished       = 0;
        int numBufferedRecords = 0;

        bslma::TestAllocator             ta;
        ball::TestObserver               to(bsl::cout, &ta);
        const ball::TestObserver        *TO = &to;
        ball::LoggerManagerConfiguration lmc;
        lmc.setTriggerMarkers(
               BloombergLP::ball::LoggerManagerConfiguration::BAEL_NO_MARKERS);

        ball::LoggerManager::initSingleton(&to, lmc, &ta);
        ball::LoggerManager& manager = ball::LoggerManager::singleton();
        for (int i = 0; i < (int) thresholds.size(); ++i) {
            ball::Category *category =
                          manager.setCategory("TestCategory",
                                              thresholds[i].recordLevel(),
                                              thresholds[i].passLevel(),
                                              thresholds[i].triggerLevel(),
                                              thresholds[i].triggerAllLevel());

            // Test with no rules.
            for (int j = 0; j < NUM_VALUES; ++j) {
                ball::Record *record = manager.getLogger().getRecord("X", 1);

                // Figure out the various thresholds.  Note that we have only
                // one thread, so trigger is equivalent to triggerAll.
                bool buffer  = VALUES[j] <= thresholds[i].recordLevel();
                bool pass    = VALUES[j] <= thresholds[i].passLevel();
                bool trigger = VALUES[j] <= thresholds[i].triggerLevel()
                            || VALUES[j] <= thresholds[i].triggerAllLevel();

                manager.getLogger().logMessage(*category, VALUES[j], record);

                if (buffer) { ++numBufferedRecords; }
                if (pass)   { ++numPublished; }
                if (trigger) {
                    numPublished += numBufferedRecords;
                    numBufferedRecords = 0;
                }
                LOOP2_ASSERT(i,j, numPublished == TO->numPublishedRecords());
            }

            // Test with rule that does not apply.
            manager.addRule(ball::Rule("JUNK", 255, 255, 255, 255));
            for (int j = 0; j < NUM_VALUES; ++j) {
                ball::Record *record = manager.getLogger().getRecord("X", 1);

                // Figure out the various thresholds.  Note that we have only
                // one thread, so trigger is equivalent to triggerAll.
                bool buffer  = VALUES[j] <= thresholds[i].recordLevel();
                bool pass    = VALUES[j] <= thresholds[i].passLevel();
                bool trigger = VALUES[j] <= thresholds[i].triggerLevel()
                            || VALUES[j] <= thresholds[i].triggerAllLevel();

                manager.getLogger().logMessage(*category, VALUES[j], record);
                if (buffer) { ++numBufferedRecords; }
                if (pass)   { ++numPublished; }
                if (trigger) {
                    numPublished += numBufferedRecords;
                    numBufferedRecords = 0;
                }
                LOOP2_ASSERT(i,j, numPublished == TO->numPublishedRecords());
            }

            // Test with a rule that does apply.
            for (int j = 0; j < (int) thresholds.size(); ++j) {
                manager.removeAllRules();
                ball::Rule rule("Test*",
                               thresholds[j].recordLevel(),
                               thresholds[j].passLevel(),
                               thresholds[j].triggerLevel(),
                               thresholds[j].triggerAllLevel());
                manager.addRule(rule);
                for (int k = 0; k < NUM_VALUES; ++k) {
                    ball::Record *record = manager.getLogger().getRecord("X",1);

                    // Each level is the minimum severity (maximum value)
                    // between the rule's threshold and the categories
                    // threshold.
                    int recordLvl  = bsl::max(thresholds[i].recordLevel(),
                                              thresholds[j].recordLevel());
                    int passLvl    = bsl::max(thresholds[i].passLevel(),
                                              thresholds[j].passLevel());
                    int triggerLvl = bsl::max(thresholds[i].triggerLevel(),
                                              thresholds[j].triggerLevel());
                    int trigAllLvl = bsl::max(thresholds[i].triggerAllLevel(),
                                              thresholds[j].triggerAllLevel());

                    // Figure out the various thresholds.  Note that we have
                    // only one thread, so trigger is equivalent to triggerAll.
                    bool buffer  = VALUES[k] <= recordLvl;
                    bool pass    = VALUES[k] <= passLvl;
                    bool trigger = VALUES[k] <= triggerLvl
                                || VALUES[k] <= trigAllLvl;

                    manager.getLogger().logMessage(*category,
                                                   VALUES[k],
                                                   record);

                    if (buffer) { ++numBufferedRecords; }
                    if (pass)   { ++numPublished; }
                    if (trigger) {
                        numPublished += numBufferedRecords;
                        numBufferedRecords = 0;
                    }
                    LOOP3_ASSERT(i, j, k,
                                 numPublished == TO->numPublishedRecords());
                }
            }
            manager.removeAllRules();
        }

        // Verify that the 3-argument 'logMessage' does not leak records when
        // records are discarded without being published.  (DRQS 64132363).

        if (verbose) cout << "Verify 'logMessage' does not leak records."
                          << endl;
        {
            ball::Category& category = manager.defaultCategory();

            ASSERT(0 == manager.getLogger().numRecordsInUse());

            ball::Record *r1 = manager.getLogger().getRecord("X", 1);

            ASSERT(1 == manager.getLogger().numRecordsInUse());

            ball::Record *r2 = manager.getLogger().getRecord("X", 1);

            ASSERT(2 == manager.getLogger().numRecordsInUse());

            manager.getLogger().logMessage(category, 1 , r1);

            ASSERT(1 == manager.getLogger().numRecordsInUse());

            manager.getLogger().logMessage(category, 254, r2);

            ASSERT(0 == manager.getLogger().numRecordsInUse());

        }

        ball::LoggerManager::shutDownSingleton();

      } break;
      case 28: {
        // --------------------------------------------------------------------
        // TESTING: isCategoryEnabled (RULE BASED LOGGING)
        //
        // Concerns:
        //   * That 'isCategoryEnabled' behaves correctly with respect to the
        //     rule based logging.
        //   * That the 'isCategoryEnabled' method is using the current
        //     installed rules when determining whether a category is enabled.
        //
        // Plan:
        //   Create a series of test threshold values and a series of
        //   threshold-aggregate values.  Test the 'isCategoryEnabled' method
        //   for each test threshold value without a logger manager.  Then
        //   create a logger manager.  For each threshold-aggregate value:
        //   create a category with that threshold-aggregate value, test the
        //   'isCategoryEnabled' method with a variety of severity levels
        //   without a logging rule, then for each test threshold-aggregate
        //   value, create a logging rule that applies to the category and test
        //   the 'isCategoryEnabled' method.  Verify that the
        //   'isCategoryEnabled' method behaves correctly given the categories
        //   threshold-aggregate, the rule's threshold-aggregate, and the
        //   supplied severity.
        //
        // Testing:
        //   bool isCategoryEnabled(const ball::Category *, int) const;
        // --------------------------------------------------------------------

        if (verbose)
            bsl::cout << bsl::endl
                      << "isCategoryEnabled\n"
                      << "==================\n";
        using namespace BloombergLP;

        int VALUES[] = { 1,
                         ball::Severity::BAEL_FATAL - 1,
                         ball::Severity::BAEL_FATAL,
                         ball::Severity::BAEL_FATAL + 1,
                         ball::Severity::BAEL_ERROR - 1,
                         ball::Severity::BAEL_ERROR,
                         ball::Severity::BAEL_ERROR + 1,
                         ball::Severity::BAEL_WARN - 1,
                         ball::Severity::BAEL_WARN,
                         ball::Severity::BAEL_WARN + 1,
                         ball::Severity::BAEL_INFO - 1,
                         ball::Severity::BAEL_INFO,
                         ball::Severity::BAEL_INFO + 1,
                         ball::Severity::BAEL_DEBUG - 1,
                         ball::Severity::BAEL_DEBUG,
                         ball::Severity::BAEL_DEBUG + 1,
                         ball::Severity::BAEL_TRACE - 1,
                         ball::Severity::BAEL_TRACE,
                         ball::Severity::BAEL_TRACE + 1
        };
        const int NUM_VALUES = sizeof (VALUES)/sizeof(*VALUES);

        bsl::vector<Thresholds> thresholds;
        for (int i = 0; i < NUM_VALUES; ++i) {
            thresholds.push_back(Thresholds(VALUES[i], 255, 255, 255));
            thresholds.push_back(Thresholds(255, VALUES[i], 255, 255));
            thresholds.push_back(Thresholds(255, 255, VALUES[i], 255));
            thresholds.push_back(Thresholds(255, 255, 255, VALUES[i]));
        }

        if (veryVerbose) {
            bsl::cout << "\tTest with and without an applicable rule."
                      << bsl::endl;
        }

        bslma::TestAllocator ta(veryVeryVerbose);
        ball::TestObserver   testObserver(bsl::cout, &ta);
        ball::LoggerManagerConfiguration lmc;
        ball::LoggerManager::initSingleton(&testObserver, lmc, &ta);
        ball::LoggerManager& manager = ball::LoggerManager::singleton();

        // Create a test category for each test threshold-aggregate
        for (int i = 0; i < (int) thresholds.size(); ++i) {
            ball::Category *category =
                          manager.setCategory("TestCategory",
                                              thresholds[i].recordLevel(),
                                              thresholds[i].passLevel(),
                                              thresholds[i].triggerLevel(),
                                              thresholds[i].triggerAllLevel());

            // Test 'isCategoryEnabled' with no rules.
            for (int j = 0; j < NUM_VALUES; ++j) {
                bool enabled =
                           VALUES[j] <= Thresholds::maxLevel(thresholds[i]);

                LOOP2_ASSERT(i, j,
                             enabled ==
                             manager.isCategoryEnabled(category, VALUES[j]));
            }

            // Test with rule that does not apply.
            manager.addRule(ball::Rule("JUNK", 255, 255, 255, 255));
            for (int j = 0; j < NUM_VALUES; ++j) {
                bool enabled =
                        VALUES[j] <= Thresholds::maxLevel(thresholds[i]);
                LOOP2_ASSERT(i, j,
                             enabled ==
                             manager.isCategoryEnabled(category, VALUES[j]));
            }

            // Test with a rule that does apply.
            for (int j = 0; j < (int) thresholds.size(); ++j) {
                manager.removeAllRules();
                ball::Rule rule("Test*",
                               thresholds[j].recordLevel(),
                               thresholds[j].passLevel(),
                               thresholds[j].triggerLevel(),
                               thresholds[j].triggerAllLevel());
                manager.addRule(rule);
                for (int k = 0; k < NUM_VALUES; ++k) {
                    int maxLevel =
                        bsl::max(Thresholds::maxLevel(thresholds[i]),
                                 Thresholds::maxLevel(thresholds[j]));
                    bool expectedEnabled = VALUES[k] <= maxLevel;
                    bool enabled = manager.isCategoryEnabled(category,
                                                             VALUES[k]);
                    LOOP3_ASSERT(i, j, k, enabled == expectedEnabled);
                    if (enabled != expectedEnabled) {
                        P_(thresholds[i]); P_(thresholds[j]); P(enabled);
                    }
                }

            }
            manager.removeAllRules();
        }
        ball::LoggerManager::shutDownSingleton();

      } break;
      case 27: {
        // --------------------------------------------------------------------
        // TESTING 'ball::LoggerManager' CONSTRUCTOR
        //
        // Concerns:
        //   We are concerned that the 'ball::LoggerManager' constructor
        //   uses the global allocator and propagates it where appropriate
        //   when no allocator is explicitly supplied at construction.
        //
        // Plan:
        //   Create an instance of the 'ball::LoggerManager' by calling its
        //   constructor directly, but do *not* specify an allocator.  Using
        //   appropriate asserts (1) following construction, (2) after creating
        //   a category, and (3) after logging a test message, verify that the
        //   global allocator is used throughout.
        //
        //   Note that the 'ball::LoggerManager' constructor is 'public' now,
        //   but the intention is to make it 'private' in the future.  As such,
        //   this is the *only* test case in this test driver which should call
        //   the constructor directly.  Also in view of the future
        //   privatization of the constructor, more thorough (indirect) testing
        //   of it is left to other test cases.
        //
        // Testing:
        //   ball::LoggerManager(Obs*, const Config&, ba = 0);
        //   ~ball::LoggerManager();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'ball::LoggerManager' constructor" << endl
                         << "========================================" << endl;

        bslma::TestAllocator defaultTestAllocator(veryVeryVerbose);
        bslma::TestAllocator globalTestAllocator(veryVeryVerbose);
        bslma::TestAllocator objectTestAllocator(veryVeryVerbose);

        bslma::TestAllocator *DA = &defaultTestAllocator;
        bslma::TestAllocator *GA = &globalTestAllocator;
        bslma::TestAllocator *OA = &objectTestAllocator;

        bslma::DefaultAllocatorGuard taGuard(DA);
        bslma::Default::setGlobalAllocator(GA);

        ball::LoggerManagerConfiguration mLMC;

        const bsls::Types::Int64 NUM_BLOCKS_DFLT_ALLOC = DA->numBlocksInUse();
        const bsls::Types::Int64 NUM_BLOCKS_GLOB_ALLOC = GA->numBlocksInUse();

        ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);
        {
            ball::TestObserver testObserver(bsl::cout, OA);

            ASSERT(!Obj::isInitialized());
            Obj manager(&testObserver, mLMC);
            ASSERT( Obj::isInitialized());
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(NUM_BLOCKS_GLOB_ALLOC <  GA->numBlocksInUse());

            Obj& mLM = Obj::singleton();

            ASSERT(&testObserver == mLM.observer());
            ASSERT(1             == mLM.numCategories());

            // add a category

            const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
            ASSERT(cat);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
            ASSERT(64  == cat->recordLevel());
            ASSERT(48  == cat->passLevel());
            ASSERT(32  == cat->triggerLevel());
            ASSERT(16  == cat->triggerAllLevel());

            // log a test message

            if (veryVerbose) testObserver.setVerbose(1);

            ball::Logger& lgr = mLM.getLogger();
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(0 == testObserver.numPublishedRecords());
            const char *MESSAGE = "scoped guard test";
            const int   LINE    = L_ + 1;
            lgr.logMessage(*cat, cat->passLevel(), __FILE__, LINE, MESSAGE);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(1 == testObserver.numPublishedRecords());
            const Rec& R = testObserver.lastPublishedRecord();
            if (veryVerbose) cout << R << endl;

            const Attr& A = R.fixedFields();
            ASSERT(0                == bsl::strcmp("Bloomberg.Bae",
                                                   A.category()));
            ASSERT(A.severity() == cat->passLevel());
            ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
            ASSERT(LINE         == A.lineNumber());
            ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

            const List& L = R.userFields();
            ASSERT(0 == L.length());
        }
        ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);

      } break;
      case 26: {
        // --------------------------------------------------------------------
        // TESTING LOW-LEVEL LOGGING
        //
        // Concerns:
        //: 1 Declaring the 'ball::LoggerManagerScopedGuard' results in a new
        //:   handler being installed in the 'bsls' logger
        //:
        //: 2 Writing a low-level log results in a proper log message being
        //:   sent to the 'ball::TestObserver'
        //:
        //: 3 No 'publish' message needs to be called for low-level logs to be
        //:   passed to the observer
        //:
        //: 4 Destruction of the 'ball::LoggerManagerScopedGuard' results in the
        //:   original low-level handler being installed
        //
        // Plan:
        //: 1 Create a scoped guard
        //:
        //: 2 Confirm the low-level 'bsls' handler has been changed
        //:
        //: 3 Write a low-level log to the 'bsls' logger.
        //:
        //: 4 Confirm that the singleton's logger manager has had a new
        //:   category added.
        //:
        //: 5 Confirm that the record was passed to the observer without
        //:   requiring a use of 'publish'.  This ensures that the default
        //:   category and severity work without needing any user intervention.
        //:
        //: 6 Confirm that all the values in the passed record are the expected
        //:   values.
        //:
        //: 7 Allow the scoped guard to fall out of scope.
        //:
        //: 8 Confirm that the old low-level log message handler was properly
        //:   reinstated after the scoped guard's destruction.
        //
        // Testing:
        //   LOW-LEVEL LOGGING
        // --------------------------------------------------------------------
        if (verbose)
            cout << endl << "TESTING LOW-LEVEL LOGGING" << endl
                         << "=========================" << endl;

#if BDE_VERSION_MINOR >= 25
        // TBD: This test is disabled for BDE 2.23.  Ensuring it will be
        // re-enabled in the future.

        // Saving low-level handler
        const bsls::Log::LogMessageHandler oldBslsHandler =
                                                bsls::Log::logMessageHandler();
        {
            if (verbose) cout << "\tCreate the test observer." << endl;
            ball::TestObserver testObserver(cout);

            if (verbose) cout << "\tCreate scoped guard." << endl;
            ball::LoggerManagerConfiguration configuration;
            ball::LoggerManagerScopedGuard guard(&testObserver, configuration);

            const int oldNumCategories
                             = ball::LoggerManager::singleton().numCategories();

            if (verbose) cout << "\tConfirm new low-level handler." << endl;
            ASSERT(bsls::Log::logMessageHandler() != oldBslsHandler);

            if (verbose) cout << "\tWrite a low-level log message." << endl;
            const char * const file    = "TestFile.cpp";
            const int          line    = 507;
            const char * const message = "Some test message.";
            bsls::Log::logMessage(file, line, message);

            if (verbose) cout << "\tConfirm a new category created." << endl;
            LOOP2_ASSERT(oldNumCategories + 1,
                         ball::LoggerManager::singleton().numCategories(),
                         oldNumCategories + 1
                           == ball::LoggerManager::singleton().numCategories());

            if(verbose) cout << "\tConfirm record passed directly." << endl;
            LOOP_ASSERT(testObserver.numPublishedRecords(),
                        1 == testObserver.numPublishedRecords());

            if(verbose) cout << "\tConfirm values in record." << endl;
            const ball::Record& record = testObserver.lastPublishedRecord();
            const ball::RecordAttributes& attributes = record.fixedFields();

            LOOP2_ASSERT("BSLS.LOG", attributes.category(),
                        0 == strcmp("BSLS.LOG", attributes.category()));

            LOOP2_ASSERT(ball::Severity::BAEL_ERROR,
                         attributes.severity(),
                         ball::Severity::BAEL_ERROR == attributes.severity());

            LOOP2_ASSERT(file,
                         attributes.fileName(),
                         0 == strcmp(file, attributes.fileName()));

            LOOP2_ASSERT(line,
                         attributes.lineNumber(),
                         line == attributes.lineNumber());

            LOOP2_ASSERT(message,
                         attributes.message(),
                         0 == strcmp(message, attributes.message()));

            if(veryVerbose) cout << "Message: " << attributes.message()<< endl;

            if(verbose) cout << "\tDestroy scoped guard." << endl;
            // Let guard fall out of this scope
        }

        {
            if(verbose) cout << "\tConfirm old handler restored." << endl;
            ASSERT(bsls::Log::logMessageHandler() == oldBslsHandler);
        }
#endif
      } break;
      case 25: {
        // --------------------------------------------------------------------
        // TESTING 'ball::LoggerManagerScopedGuard'
        //
        // Concerns:
        //   We want to make sure that 'ball::LoggerManagerScopedGuard' calls
        //   'initSingleton' on its construction and 'shutDownSingleton' on
        //   its destruction.
        //
        // Plan:
        //   We create an auxiliary class that checks the initialization
        //   status of the logger manager singleton in both its constructor
        //   and destructor.  Objects of this class created before the logger
        //   manager singleton should not see the logger manager, whereas
        //   objects created after the logger manager singleton should always
        //   report the existence of the logger manager.
        //
        // Testing:
        //   ball::LoggerManagerScopedGuard(Obs*, const Config&, ba = 0);
        //   ~ball::LoggerManagerScopedGuard();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'ball::LoggerManagerScopedGuard'" << endl
                         << "=======================================" << endl;

        bslma::TestAllocator defaultTestAllocator(veryVeryVerbose);
        bslma::TestAllocator globalTestAllocator(veryVeryVerbose);
        bslma::TestAllocator objectTestAllocator(veryVeryVerbose);

        bslma::TestAllocator *DA = &defaultTestAllocator;
        bslma::TestAllocator *GA = &globalTestAllocator;
        bslma::TestAllocator *OA = &objectTestAllocator;

        bslma::DefaultAllocatorGuard taGuard(DA);
        bslma::Default::setGlobalAllocator(GA);

        class InitializationVerifier {

            bool d_isInitializedFlag;  // 'true' if logger manager is
                                       // available upon destruction

          public:
            // CREATORS
            InitializationVerifier(bool isInitializedFlag)
            : d_isInitializedFlag(isInitializedFlag)
            {
                ASSERT(d_isInitializedFlag == Obj::isInitialized());
            }

            ~InitializationVerifier()
            {
                ASSERT(d_isInitializedFlag == Obj::isInitialized());
            }
        };

        ball::LoggerManagerConfiguration mLMC;

        const bsls::Types::Int64 NUM_BLOCKS_DFLT_ALLOC = DA->numBlocksInUse();
        const bsls::Types::Int64 NUM_BLOCKS_GLOB_ALLOC = GA->numBlocksInUse();

        ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);
        {
            ball::TestObserver testObserver(bsl::cout, OA);

            InitializationVerifier ivPre(false);
            ASSERT(false == Obj::isInitialized());

            ball::LoggerManagerScopedGuard guard(&testObserver, mLMC);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(NUM_BLOCKS_GLOB_ALLOC <  GA->numBlocksInUse());

            ASSERT(true == Obj::isInitialized());
            InitializationVerifier ivPost(true);

            Obj& mLM = Obj::singleton();

            ASSERT(&testObserver == mLM.observer());
            ASSERT(1             == mLM.numCategories());

            // add a category

            const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
            ASSERT(cat);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
            ASSERT(64  == cat->recordLevel());
            ASSERT(48  == cat->passLevel());
            ASSERT(32  == cat->triggerLevel());
            ASSERT(16  == cat->triggerAllLevel());

            // log a test message

            if (veryVerbose) testObserver.setVerbose(1);

            ball::Logger& lgr = mLM.getLogger();
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(0 == testObserver.numPublishedRecords());
            const char *MESSAGE = "scoped guard test";
            const int   LINE    = L_ + 1;
            lgr.logMessage(*cat, cat->passLevel(), __FILE__, LINE, MESSAGE);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ASSERT(1 == testObserver.numPublishedRecords());
            const Rec& R = testObserver.lastPublishedRecord();
            if (veryVerbose) cout << R << endl;

            const Attr& A = R.fixedFields();
            ASSERT(0                == bsl::strcmp("Bloomberg.Bae",
                                                   A.category()));
            ASSERT(A.severity() == cat->passLevel());
            ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
            ASSERT(LINE         == A.lineNumber());
            ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

            const List& L = R.userFields();
            ASSERT(0 == L.length());
        }
        ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);

      } break;
      case 24: {
        // --------------------------------------------------------------------
        // TESTING STATIC 'getRecord' and 'logMessage'
        //
        // Concerns:
        //   The 'getRecord' and 'logMessage' should work correctly without a
        //   'ball::LoggerManager' instance.
        //
        // Plan:
        //
        // Testing:
        //   static ball::Record *getRecord(const char *file, int line);
        //   static void logMessage(int severity, ball::Record *record);
        // --------------------------------------------------------------------

        if (verbose) cout << endl
                          << "Test static 'getRecord' and 'logMessage'\n"
                          << "========================================\n";

         bslma::TestAllocator testAllocator(veryVeryVerbose);
         bslma::DefaultAllocatorGuard guard(&testAllocator);

         ball::Record* record1 = Obj::getRecord(F_, L_);
         ball::Record* record2 = Obj::getRecord(F_, L_);
         ball::Record* record3 = Obj::getRecord(F_, L_);
         ball::Record* record4 = Obj::getRecord(F_, L_);
         ball::Record* record5 = Obj::getRecord(F_, L_);
         ball::Record* record6 = Obj::getRecord(F_, L_);

         // The first three messages are set to strings without embedded '\0'.

         record1->fixedFields().setMessage("No Logger Manager!");
         record2->fixedFields().setMessage("No Logger Manager!");
         record3->fixedFields().setMessage("No Logger Manager!");

         // The next three messages are set to strings with embedded '\0'.

         record4->fixedFields().messageStreamBuf().pubseekpos(0);
         record4->fixedFields().messageStreamBuf().sputn(
                                               "No Logger\0 Manager\0 4!", 22);

         record4->fixedFields().messageStreamBuf().pubseekpos(0);
         record4->fixedFields().messageStreamBuf().sputn(
                                               "No Logger\0 Manager\0 5!", 22);

         record4->fixedFields().messageStreamBuf().pubseekpos(0);
         record4->fixedFields().messageStreamBuf().sputn(
                                               "No Logger\0 Manager\0 6!", 22);

#ifdef BSLS_PLATFORM_OS_UNIX
         fflush(stderr);
         bsl::string filename = tempnam(0, "ball_loggermanager");
         int fd = creat(filename.c_str(), 0777);
         ASSERT(fd != -1);
         int saved_stderr_fd = dup(2);
         dup2(fd, 2);
#endif

         bsl::stringstream os;
         bsl::streambuf *cerrBuf = bsl::cerr.rdbuf();
         bsl::cerr.rdbuf(os.rdbuf());

         Obj::logMessage(ball::Severity::BAEL_TRACE, record1);
         Obj::logMessage(ball::Severity::BAEL_DEBUG, record2);
         Obj::logMessage(ball::Severity::BAEL_INFO,  record3);
         Obj::logMessage(ball::Severity::BAEL_WARN,  record4);
         Obj::logMessage(ball::Severity::BAEL_ERROR, record5);
         Obj::logMessage(ball::Severity::BAEL_FATAL, record6);

#ifdef BSLS_PLATFORM_OS_UNIX
         fflush(stderr);
         dup2(saved_stderr_fd, 2);

         bsl::ifstream fs(filename.c_str(), bsl::ifstream::in);
         int numLines = 0;
         bsl::string line;
         while (getline(fs, line)) {
             if (veryVerbose) bsl::cout << line << bsl::endl;
             ++numLines;
         }
         fs.close();
         ASSERT(12 == numLines);
         unlink(filename.c_str());
#endif

         ASSERT("" == os.str());
         bsl::cerr.rdbuf(cerrBuf);

      } break;
      case 23: {
        // --------------------------------------------------------------------
        // TESTING RULE MANIPULATION (TEMPORARILY LEFT BLANK)
        // --------------------------------------------------------------------

      } break;
      case 22: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO 'ball::LoggerManager::lookupCategory'
        //
        // Concerns:
        //   That 'lookupCategory' is thread-safe when a category name filter
        //   is in effect.
        //
        // Plan:
        //   Initialize the logger manager with a 'toLower' category name
        //   filter.  Add 4 categories whose names are all in lowercase.
        //   In 4 concurrent threads, lookup those categories using their
        //   similar uppercase names.  Assert in the helper thread function
        //   'workerThread29' that the category is found.
        //
        // Testing:
        //   const ball::Category *lookupCategory(const char *categoryName);
        // --------------------------------------------------------------------
        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO "
                 << "PRIMARY 'ball::LoggerManager::lookupCategory'" << endl
                 << "============================="
                 << "============================================" << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_22;

        ball::DefaultObserver observer(cout);
        ball::LoggerManagerConfiguration configuration;
        Cnf nameFilter(&toLower);
        configuration.setCategoryNameFilterCallback(nameFilter);
        ball::LoggerManagerScopedGuard guard(&observer, configuration);
        manager = &Obj::singleton();

        const int NUM_DATA = sizeof LC / sizeof *LC;
        ASSERT(NUM_DATA == NUM_THREADS);
        int ti;
        for (ti = 0; ti < NUM_THREADS; ++ti) {
            ASSERT(manager->setCategory(LC[ti]));
        }

        ThreadData data[NUM_THREADS];

        for (ti = 0; ti < NUM_THREADS; ++ti) {
            data[ti].index = ti;
        }

        bdlqq::ThreadUtil::Handle threads[NUM_THREADS];

        for (ti = 0; ti < NUM_THREADS; ++ti) {
            bdlqq::ThreadUtil::create(&threads[ti],
                                     workerThread29,
                                     (void*)&data[ti]);
        }

        for (ti = 0; ti < NUM_THREADS; ++ti) {
            bdlqq::ThreadUtil::join(threads[ti]);
        }

      } break;
      case 21: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO 'ball::LoggerManager::initSingleton()':
        //   Verify Concurrent access to 'initSingleton()'.
        //
        // Concerns:
        //   That multiple threads can safely invoke 'initSingleton' with
        //   different observers and configuration objects, and the resulting
        //   logger manager singleton must be initialized with one of these
        //   observers and configuration objects.
        //
        // Plan:
        //   Create several threads, each of which invoke 'initSingleton' with
        //   different observers and configuration objects.
        //
        // Testing:
        //   const ball::Category *setCategory(const char *categoryName);
        // --------------------------------------------------------------------
        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO "
                 << "PRIMARY 'ball::LoggerManager::initSingleton'" << endl
                 << "============================="
                 << "===========================================" << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_21;

        ThreadData data[NUM_THREADS];

        int i;
        for (i = 0; i < NUM_THREADS; ++i) {
            data[i].index = i;
            data[i].observer = new ball::DefaultObserver(bsl::cout);
        }

        bdlqq::ThreadUtil::Handle threads[NUM_THREADS];

        for (i = 0; i < NUM_THREADS; ++i) {
            bdlqq::ThreadUtil::create(&threads[i],
                                     workerThread28,
                                     (void*)&data[i]);
        }

        for (i = 0; i < NUM_THREADS; ++i) {
            bdlqq::ThreadUtil::join(threads[i]);
        }

        Obj& mLM = Obj::singleton();
        for (i = 0; i < NUM_THREADS; ++i) {
            LOOP_ASSERT(i, &mLM == data[i].manager);
            if (mLM.observer() == data[i].observer) {
                if (veryVerbose) cout << "Thread " << i << " initialized "
                                       << "the logger manager singleton"
                                       << endl;

                ASSERT(mLM.defaultRecordThresholdLevel()     == data[i].index);
                ASSERT(mLM.defaultPassThresholdLevel()       == data[i].index);
                ASSERT(mLM.defaultTriggerThresholdLevel()    == data[i].index);
                ASSERT(mLM.defaultTriggerAllThresholdLevel() == data[i].index);

                break;
            }
        }

        ASSERT(NUM_THREADS != i);

        for (i = 0; i < NUM_THREADS; ++i) {
            delete data[i].observer;
        }

      } break;
      case 20: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO 'ball::LoggerManager::setCategory':
        //   Verify Concurrent access to 'setCategory'.
        //
        // Concerns:
        //   That multiple threads can safely invoke 'setCategory' with the
        //   same category argument.
        //
        // Plan:
        //   Create several threads, each of which invoke 'setCategory' with
        //   the same category argument.
        //
        // Testing:
        //   const ball::Category *setCategory(const char *categoryName);
        // --------------------------------------------------------------------
        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO"
                 << "'ball::LoggerManager::setCategory'" << endl
                 << "==========================================" << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_20;

        ball::DefaultObserver observer(cout);
        ball::LoggerManagerConfiguration configuration;
        ball::LoggerManagerScopedGuard guard(&observer, configuration);
        manager = &Obj::singleton();

        executeInParallel(NUM_THREADS, workerThread27);

      } break;
      case 19: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO 'd_defaultLoggers':
        //   Verify concurrent access to 'd_defaultLoggers' (a class member
        //   of 'ball::LoggerManager').
        //
        // Concerns:
        //   Verify concurrent access to 'd_defaultLoggers' (which maintains
        //   the registered loggers).
        //
        // Plan:
        //  'd_defaultLoggers' is primarily manipulated by 'setLogger' and
        //  'getLogger'.  We create several threads, half of them, will
        //  perform following operations.
        //    -allocate a logger (say 'logger');
        //    -run a tight loop; in each iteration, invoke
        //     'setLogger(logger)' followed by 'getLogger'; assert that
        //     the logger returned by 'getLogger' is same as the 'logger'.
        //
        //  and the rest half will perform following operations.
        //    -get the default logger by calling 'getLogger';
        //    -run a tight loop; in each iteration, invoke 'getLogger'
        //     and assert that it returns the default logger.
        //
        // Testing:
        //   void setLogger(ball::Logger *logger);
        //   ball::Logger& getLogger();
        // --------------------------------------------------------------------

        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO 'd_defaultLoggers'" << endl
                 << "===============================================" << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_19;

        ball::DefaultObserver            observer(cout);
        ball::LoggerManagerConfiguration configuration;
        static bslma::TestAllocator      ta(veryVeryVerbose);

        ball::LoggerManagerScopedGuard guard(&observer, configuration, &ta);
        manager = &Obj::singleton();

        executeInParallel(NUM_THREADS, workerThread26);

      } break;
      case 18: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS TO 'd_loggers':
        //   Verify concurrent access to 'd_loggers' (a class member
        //   of 'ball::LoggerManager').
        //
        // Concerns:
        //   Verify concurrent access to 'd_loggers' (which maintains
        //   the allocated loggers).
        //
        // Plan:
        //  'd_loggers' is primarily manipulated by 'allocateLogger',
        //  'logMessage' (specifically, when the logged message is severe
        //  enough to cause a trigger-all) and 'deallocateLogger'.  We
        //  create several threads, each, in a tight loop, will perform
        //  following operations.
        //    -allocate a logger (say 'logger');
        //    -log a message to 'logger', causing a trigger-all;
        //    -deallocate the 'logger'
        //
        //  Finally join all the threads and verify the state.
        //
        // Testing:
        //   ball::Logger *allocateLogger(ball::RecordBuffer *buffer);
        //   void deallocateLogger(ball::Logger *logger);
        //   void logMessage(const ball::Category&  category,
        //                   int                   severity,
        //                   const char           *fileName,
        //                   int                   lineNumber,
        //                   const char           *message);
        // --------------------------------------------------------------------
        if (verbose) {
            cout << endl
                 << "TESTING CONCURRENT ACCESS TO 'd_loggers'" << endl
                 << "========================================" << endl;
        }

        using namespace BAEL_LOGGERMANAGER_TEST_CASE_18;

        my_publishCountingObserver      observer;
        ball::LoggerManagerConfiguration configuration;
        configuration.setTriggerMarkers(
               BloombergLP::ball::LoggerManagerConfiguration::BAEL_NO_MARKERS);

        static bslma::TestAllocator      ta(veryVeryVerbose);
        ball::LoggerManagerScopedGuard guard(&observer, configuration, &ta);
        Obj& localManager = Obj::singleton();
        manager = &localManager;

        // for simplicity we log the messages with severity, that will
        // cause *record* followed by *trigger-all*.
        cat = localManager.addCategory("test-category",
                                  ball::Severity::BAEL_ERROR,  // record
                                  ball::Severity::BAEL_FATAL,  // passthrough
                                  ball::Severity::BAEL_FATAL,  // trigger
                                  ball::Severity::BAEL_ERROR); // triggerAll

        executeInParallel(NUM_THREADS, workerThread25);
        int c = observer.publishCount();

        // there are 'NUM_THREADS', each of which logs 'NUM_ITERATIONS'
        // messages.  Also note that each logged message is eventually
        // published (either by trigger-all caused by the same thread or
        // by different thread).  Thus the publish count should be equal to
        // 'NUM_THREADS * NUM_ITERATIONS'.
        LOOP_ASSERT(c, c == NUM_THREADS * NUM_ITERATIONS);

      } break;
      case 17: {
        // --------------------------------------------------------------------
        // PERFORMANCE TEST
        //
        // Concerns:
        //   'logMessage' should run efficiently when no record is stored,
        //   and relatively efficient when records need to be stored.
        //
        // Plan:
        //   Invoke 'logMessage' in a loop for the default number of times
        //   or else for an integral multiple thereof as specified in the
        //   second argument ('argv[2]') to the executable.
        //
        // Testing:
        //   performance of 'logMessage'
        // --------------------------------------------------------------------

        ball::TestObserver testObserver(cout);

        Cnf nameFilter(&toLower);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&inheritThresholdLevels);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("FACTORIAL");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "n!");

        ball::Logger::UserPopulatorCallback populator(&myPopulator);

        ball::LoggerManagerConfiguration mLMC;
        mLMC.setCategoryNameFilterCallback(nameFilter);
        mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
        mLMC.setUserFields(schema, populator);

        ball::LoggerManagerScopedGuard guard(&testObserver, mLMC);
        Obj& mLM = Obj::singleton();
        ball::Logger&        lgr = mLM.getLogger();

        Cat *cat = mLM.addCategory("PerfTest", 200, 150, 100, 50);

        const char *currentFile = __FILE__;

        // Add some categories
        const int DEFAULT = 0;   // keep default standard
        int SIZE = verbose && atoi(argv[2]) > DEFAULT
                   ? atoi(argv[2])
                   : DEFAULT;

        if (verbose) cout << "SIZE = " << SIZE << endl;

        //                      vvvv Adjust Here to change default runtime.
        const int size = SIZE * 200;

        const int NUM_FEEDBACKS = 50;
        int feedback = size / NUM_FEEDBACKS;
        if (feedback <= 0) feedback = 1;

        if (verbose) cout << "Performance test for 'logMessage' "
                          << size << " times. " << endl;
        {
            if (verbose) cerr << "     Baseline -- simple calculations only:"
                              << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            int m = 29;         // a randomly picked prime number

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                m = (m * i + m) % INT_MAX;
            }
            if (verbose) cerr << "END" << endl;

            if (verbose) cerr << "     Less severe than record:" << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                lgr.logMessage(*cat, 250, currentFile, __LINE__, "Message");
            }
            if (verbose) cerr << "END" << endl;

            if (verbose) cerr << "     Less severe than pass but more severe"
                              << " than record:" << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                lgr.logMessage(*cat, 180, currentFile, __LINE__, "Message");
            }
            if (verbose) cerr << "END" << endl;

            if (verbose) cerr << "     Less severe than trigger but more"
                              << " severe than pass:" << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                lgr.logMessage(*cat, 120, currentFile, __LINE__, "Message");
            }
            if (verbose) cerr << "END" << endl;

            if (verbose) cerr << "     Less severe than trigger all"
                              << " but more severe than trigger:" << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                lgr.logMessage(*cat, 70, currentFile, __LINE__, "Message");
            }
            if (verbose) cerr << "END" << endl;

            if (verbose) cerr << "     more severe than trigger all:" << endl;
            if (verbose) cerr << "     "
                "----+----+----+----+----+----+----+----+----+----+" << endl;
            if (verbose) cerr << "BEGIN";

            for (int i = size - 1; i >= 0; --i)  {
                if (verbose && 0 == i % feedback) cerr << '.';
                lgr.logMessage(*cat, 20, currentFile, __LINE__, "Message");
            }
            if (verbose) cerr << "END" << endl;
        }

      } break;
      case 16: {
        // -------------------------------------------------------------------
        // TESTING allocateLogger, deallocateLogger, setLogger
        //
        // Concerns:
        //   - Default and user-supplied message buffer sizes are correct
        //   - Different loggers can be "set" and subsequently retrieved
        //   - Deallocated loggers cannot be set
        //
        // Plan:
        //   Allocate a set S of loggers having different message buffer
        //   sizes.
        //   For each s in S, verify that the logger has the expected buffer
        //   size, and can be "set" as the active logger.
        //
        // Testing:
        //   ball::Logger *allocateLogger(*buffer)
        //   ball::Logger *allocateLogger(*buffer, int msgBufSize);
        //   ball::Logger *allocateLogger(*buffer, *observer);
        //   ball::Logger *allocateLogger(*buffer, int msgBufSize, *observer);
        //   void deallocateLogger(ball::Logger *logger);
        //   void setLogger(ball::Logger *logger);
        // -------------------------------------------------------------------

        if (verbose) cout << endl << "Testing allocation and setting" << endl
                                  << "==============================" << endl;

        bslma::TestAllocator  testAllocator(veryVeryVerbose);
        bslma::DefaultAllocatorGuard guard(&testAllocator);
        bslma::Allocator    *Z = &testAllocator;
        ball::TestObserver    testObserver(cout);

        ball::LoggerManagerConfiguration mLMC;
        ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
        Obj& mLM = Obj::singleton();

        ASSERT(1 == mLM.numCategories());

        if (verbose) cout << "\tTest allocateLogger(*buffer)" << endl;
        {
            Logger *logger[NUM_LOGGERS];
            RecBuf *recBuf[NUM_LOGGERS];

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                recBuf[i] = new(*Z) ball::FixedSizeRecordBuffer(BUF_SIZE);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(testAllocator)
                    logger[i] = mLM.allocateLogger(recBuf[i]);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                mLM.setLogger(logger[i]);
                ASSERT(&mLM.getLogger() == logger[i]);
            }

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                mLM.deallocateLogger(logger[i]);
                recBuf[i]->~RecBuf();
                Z->deallocate(recBuf[i]);
            }
        }

        if (verbose)
            cout << "\tTest allocateLogger(*buffer, int msgBufSize)" << endl;
        {
            const int K = 1024;

            Logger *logger[NUM_LOGGERS];
            RecBuf *recBuf[NUM_LOGGERS];

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                recBuf[i] = new(*Z) ball::FixedSizeRecordBuffer(BUF_SIZE);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(testAllocator)
                    logger[i] = mLM.allocateLogger(recBuf[i], (i + 1) * K);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                ASSERT((i + 1) * K == logger[i]->messageBufferSize());
                mLM.setLogger(logger[i]);
                ASSERT(&mLM.getLogger() == logger[i]);
            }

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                mLM.deallocateLogger(logger[i]);
                recBuf[i]->~RecBuf();
                Z->deallocate(recBuf[i]);
            }
        }

        if (verbose)
            cout << "\tTest allocateLogger(*buffer, *observer)" << endl;
        {
            Logger *logger[NUM_LOGGERS];
            RecBuf *recBuf[NUM_LOGGERS];
            ball::TestObserver observer(cout);

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                recBuf[i] = new(*Z) ball::FixedSizeRecordBuffer(BUF_SIZE);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(testAllocator)
                    logger[i] = mLM.allocateLogger(recBuf[i], &observer);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                mLM.setLogger(logger[i]);
                ASSERT(&mLM.getLogger() == logger[i]);
            }

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                mLM.deallocateLogger(logger[i]);
                recBuf[i]->~RecBuf();
                Z->deallocate(recBuf[i]);
            }
        }

        if (verbose)
            cout << "\tTest allocateLogger(*buffer, "
                 << "int msgBufSize, *observer)" << endl;
        {
            const int K = 1024;

            Logger *logger[NUM_LOGGERS];
            RecBuf *recBuf[NUM_LOGGERS];
            ball::TestObserver observer(cout);

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                recBuf[i] = new(*Z) ball::FixedSizeRecordBuffer(BUF_SIZE);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(testAllocator)
                    logger[i] = mLM.allocateLogger(recBuf[i],
                                                   (i + 1) * K,
                                                   &observer);
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                ASSERT((i + 1) * K == logger[i]->messageBufferSize());
                mLM.setLogger(logger[i]);
                ASSERT(&mLM.getLogger() == logger[i]);
            }

            for (int i = 0; i < NUM_LOGGERS; ++i) {
                mLM.deallocateLogger(logger[i]);
                recBuf[i]->~RecBuf();
                Z->deallocate(recBuf[i]);
            }
        }

      } break;

      case 15: {
        // --------------------------------------------------------------------
        // TESTING LOGMESSAGE LOG ORDER and TRIGGER MARKERS
        //
        // Concerns:
        //   We want to ensure that log records are published in the proper
        //   order, and with the proper trigger markers as specified by the
        //   'logOrder' and 'triggerMarkers' settings of the specified
        //   'loggerManagerConfiguration'.
        //
        // Plan:
        //   Create a category with decreasing threshold levels.  Log messages
        //   with severities less than the trigger threshold.  Then log
        //   a message above the trigger threshold and verify:
        //
        //   1) If the 'loggerManagerConfiguration' is left at default, 'LIFO'
        //      ordering of logs is used, and no trigger markers are created.
        //   2) If 'LIFO' or 'FIFO' is specified in the
        //      'loggerManagerConfiguration', the respective ordering of logs
        //      is respected.
        //   3) If 'NO_MARKERS' or 'BEGIN_END_MARKERS' is specified, the
        //      respective markers (either none, or "BEGIN" / "END" pair) are
        //      properly created.
        //
        // Testing:
        //  ^ball::Logger(Obs*, *buffer, Sch*, Pop&, Pac&, *ba);
        //  ^~ball::Logger();
        //  ^void publish(ball::Transmission::Cause cause);
        //   void logMessage(const Cat&, int, const char*, int, const char*);
        //
        //   Note: '^' indicates a private method which is tested indirectly.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing 'logMessage'" << endl
                                  << "====================" << endl;

        enum Level {
            TRIGGERALL =  32,
            TRIGGER    =  64,
            PASS       =  96,
            RECORD     = 128
        };

        typedef ball::LoggerManagerConfiguration lmc;

        static const lmc::LogOrder LOGORDER[3] = {
            lmc::BAEL_LIFO,  // default
            lmc::BAEL_FIFO,
            lmc::BAEL_LIFO
        };

        static const lmc::TriggerMarkers TRIGGERMARKERS[3] = {
            lmc::BAEL_BEGIN_END_MARKERS,         // default
            lmc::BAEL_BEGIN_END_MARKERS,
            lmc::BAEL_NO_MARKERS
        };

        static const struct {
            const char *message;
            Level       lvl;
        } DATA[] = {
            {"RECORD1",  RECORD  },
            {"PASS1",    PASS    },
            {"PASS2",    PASS    },
            {"RECORD2",  RECORD  },
            {"PASS3",    PASS    },
            {"TRIGGER1", TRIGGER },
        };

        const int NUM_LOGORDERS = sizeof LOGORDER / sizeof *LOGORDER;
        const int NUM_TRIGGERMARKERS = sizeof TRIGGERMARKERS /
                                                        sizeof *TRIGGERMARKERS;
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        int expectedNumPublished = NUM_DATA;
        for (int i = 0; i < NUM_DATA; ++i) {
            if (DATA[i].lvl != RECORD) {
                ++expectedNumPublished;
            }
        }

        bslma::TestAllocator testAllocator(veryVeryVerbose);

        for (int i = 0; i < NUM_LOGORDERS; ++i) {
            for (int j = 0; j < NUM_TRIGGERMARKERS; ++j) {

                if (veryVerbose) {
                    P_(i); P(j);
                }

                bsl::stringstream outStream;
                BAEL_LOGGERMANAGER_TEST_CASE_15::MyObserver testObserver(
                                                                    outStream);

                ball::LoggerManagerConfiguration mLMC;

                if (i) {
                    mLMC.setLogOrder(LOGORDER[i]);
                }
                if (j) {
                    mLMC.setTriggerMarkers(TRIGGERMARKERS[j]);
                }

                const int MAX_LIMIT = 1000000;
                mLMC.setDefaultRecordBufferSizeIfValid(MAX_LIMIT);

                Obj mLM(&testObserver, mLMC);

                // Set the default threshold.
                mLM.setDefaultThresholdLevels(RECORD, PASS, TRIGGER,
                                                                   TRIGGERALL);
                mLM.setCategoryThresholdsToCurrentDefaults(
                                                     &(mLM.defaultCategory()));
                const Cat& defaultCat = mLM.defaultCategory();

                // Verify default settings for threshold.
                ASSERT(RECORD     == defaultCat.recordLevel());
                ASSERT(PASS       == defaultCat.passLevel());
                ASSERT(TRIGGER    == defaultCat.triggerLevel());
                ASSERT(TRIGGERALL == defaultCat.triggerAllLevel());

                ball::Logger& LGR = mLM.getLogger();

                const int BUF_CAP = testObserver.publishCount();
                int publishCount = BUF_CAP;  // # of records published so far

                if (veryVerbose) {
                    P(publishCount);
                }

                // Generate some log messages.
                int k = 0;
                for (; k < NUM_DATA - 1; ++k) {
                    LGR.logMessage(defaultCat,
                                   DATA[k].lvl,
                                   F_,
                                   L_,
                                   DATA[k].message);
                }

                // Now log a message above trigger threshold.
                LGR.logMessage(defaultCat,
                               DATA[k].lvl,
                               F_,
                               L_,
                               DATA[k].message);
                publishCount += expectedNumPublished;

                if (TRIGGERMARKERS[j] == lmc::BAEL_BEGIN_END_MARKERS) {
                    publishCount += 2;  // 2 for the markers
                }

                if (veryVerbose) {
                    P_(publishCount); P(testObserver.publishCount());
                }
                ASSERT(publishCount == testObserver.publishCount());

                // Construct the expected stream
                stringstream ss;

                // Regular messages published
                for (k = 0; k < NUM_DATA; ++k) {
                    if (DATA[k].lvl != RECORD) {
                        ss << "Log 1 of 1 : " << DATA[k].message << "\n";
                    }
                }

                // Trigger markers if necessary.
                if (TRIGGERMARKERS[j] == lmc::BAEL_BEGIN_END_MARKERS) {
                    ss << "Log 1 of 1 : "
                          "--- BEGIN RECORD DUMP CAUSED BY TRIGGER ---\n";
                }

                // Trigger dump.
                if (LOGORDER[i] == lmc::BAEL_LIFO) {
                    for (k = NUM_DATA - 1; k >= 0; --k) {
                        ss << "Log " << NUM_DATA - k << " of " << NUM_DATA
                           << " : " << DATA[k].message << "\n";
                    }
                }
                else {
                    for (k = 0; k < NUM_DATA; ++k) {
                        ss << "Log " << k + 1 << " of " << NUM_DATA
                           << " : " << DATA[k].message << "\n";
                    }
                }

                // Trigger markers if necessary.
                if (TRIGGERMARKERS[j] == lmc::BAEL_BEGIN_END_MARKERS) {
                    ss << "Log 1 of 1 : "
                          "--- END RECORD DUMP CAUSED BY TRIGGER ---\n";
                }

                if (veryVerbose) {
                    P(ss.str());
                    P(outStream.str());
                }

                ASSERT(ss.str() == outStream.str());
            }
        }

      } break;
      case 14: {
        // --------------------------------------------------------------------
        // TESTING LOGMESSAGE
        //
        // Concerns:
        //   We want to ensure that log records are properly stored in the
        //   buffer when necessary, and are published when triggered.
        //
        // Plan:
        //   Create a category with decreasing threshold levels.  Log messages
        //   with severities less than record threshold, verify they are not
        //   stored.  Log messages with severities between record and pass
        //   thresholds, verify they are stored but not published.  Log
        //   messages with severities between pass and trigger thresholds,
        //   verify they are stored and also published.  Trigger and trigger
        //   all are verified in the process of verifying records are stored
        //   properly in the above tests.  Change the order of the threshold
        //   levels and verify the behavior is correct according to the
        //   threshold level values.
        //
        // Testing:
        //  ^ball::Logger(Obs*, *buffer, Sch*, Pop&, Pac&, *ba);
        //  ^~ball::Logger();
        //  ^void publish(ball::Transmission::Cause cause);
        //   void logMessage(const Cat&, int, const char*, int, const char*);
        //   void publish();
        //   void removeAll();
        //   char *messageBuffer();
        //   int messageBufferSize() const;
        //
        //  ^void publishAllImp(ball::Transmission::Cause cause);
        //   ball::Logger& getLogger();
        //   void publishAll();
        //
        //   Note: '^' indicates a private method which is tested indirectly.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing 'logMessage'" << endl
                                  << "====================" << endl;

        const char* testNames[] = {
            "Singleton\n---------", "Non-Singleton\n-------------"
        };

        for (int a = 0; a < 2; ++a)
        {
            if (verbose) {
                cout << testNames[a] << endl;
            }

            bslma::TestAllocator testAllocator(veryVeryVerbose);
            ball::TestObserver testObserver(cout);

            Cnf nameFilter(&toLower);

            bdlmxxx::Schema schema;
            bdlmxxx::RecordDef *p = schema.createRecord("FACTORIAL");
            p->appendField(bdlmxxx::ElemType::BDEM_INT, "n!");

            ball::Logger::UserPopulatorCallback populator(&myPopulator);

            ball::LoggerManagerConfiguration mLMC;
            mLMC.setTriggerMarkers(
               BloombergLP::ball::LoggerManagerConfiguration::BAEL_NO_MARKERS);

            const int MAX_LIMIT = 1000000;
            mLMC.setDefaultRecordBufferSizeIfValid(MAX_LIMIT);

            mLMC.setCategoryNameFilterCallback(nameFilter);
            mLMC.setUserFields(schema, populator);

            Obj* mLM_p;
            bslma::ManagedPtr<Obj> mLM_mp;
            Obj::DefaultThresholdLevelsCallback thresholdsCallback;
            if (0 == a) {
                thresholdsCallback = &inheritThresholdLevels;

                mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
                mLM_p = &(ball::LoggerManager::initSingleton(&testObserver,
                                                            mLMC));
            }
            else {
                thresholdsCallback =
                    bdlf::BindUtil::bind( &inheritThresholdLevelsUsingManager
                                       , _1
                                       , _2
                                       , _3
                                       , _4
                                       , _5
                                       , (Obj*)0);

                mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
                ball::LoggerManager::createLoggerManager(&mLM_mp,
                                                        &testObserver,
                                                        mLMC);
                mLM_p = mLM_mp.ptr();
                g_overrideManager = mLM_p;
            }

            Obj& mLM = *mLM_p;
            ASSERT(1 == mLM.numCategories());

            static const struct {
                const char *d_catName;         // category name
                int         d_recLevel;        // record level (dflt, if 0)
                int         d_passLevel;       // pass level
                int         d_trigLevel;       // trigger level
                int         d_trigAllLevel;    // trigger-all level
                int         d_expRecLevel;     // expected rec level
                int         d_expPassLevel;    // expected pass level
                int         d_expTrigLevel;    // expected trigger level
                int         d_expTrigAllLevel; // expected trigger-all level
            } DATA[] = {
{ "A",             200,    150,   100,   50,     200,   150,    100,   50    },
{ "a.B",             0,      0,     0,    0,     200,   150,    100,   50    },
{ "A.B.C.D",         0,      0,     0,    0,     200,   150,    100,   50    },
{ "B.C",           210,    160,   110,   60,     210,   160,    110,   60    },
{ "B",             220,    170,   120,   70,     220,   170,    120,   70    },
{ "B.C.A",           0,      0,     0,    0,     210,   160,    110,   60    },
{ "B.c.A",           0,      0,     0,    0,     210,   160,    110,   60    },
{ "B.A.C",           0,      0,     0,    0,     220,   170,    120,   70    },

{ "AA",              0,      0,     0,    0,       0,    64,      0,    0    },
{ "BB",              0,      0,     0,    0,       0,    64,      0,    0    },
{ "Aa.BB",           0,      0,     0,    0,       0,    64,      0,    0    },

{ "A.B.C.D.E.F.G",   0,      0,     0,    0,     200,   150,    100,   50    },
          };
          const int NUM_DATA = sizeof DATA / sizeof *DATA;

          int numCat = 1;
          for (int i = 0; i < NUM_DATA; ++i) {
              const char *CAT_NAME        = DATA[i].d_catName;
              const int   RECORD          = DATA[i].d_recLevel;
              const int   PASS            = DATA[i].d_passLevel;
              const int   TRIGGER         = DATA[i].d_trigLevel;
              const int   TRIGGERALL      = DATA[i].d_trigAllLevel;
              const int   EXPRECLEVEL     = DATA[i].d_expRecLevel;
              const int   EXPPASSLEVEL    = DATA[i].d_expPassLevel;
              const int   EXPTRIGLEVEL    = DATA[i].d_expTrigLevel;
              const int   EXPTRIGALLLEVEL = DATA[i].d_expTrigAllLevel;
              LOOP_ASSERT(i, numCat == mLM.numCategories());
              if (0 == mLM.lookupCategory(CAT_NAME)) {
                  ++numCat;
              }
              const Cat *cat = RECORD ? mLM.addCategory(CAT_NAME,
                                                        RECORD,
                                                        PASS,
                                                        TRIGGER,
                                                        TRIGGERALL)
                                      : mLM.setCategory(CAT_NAME);
              ASSERT(cat);
              LOOP_ASSERT(i, numCat == mLM.numCategories());
              if (RECORD) {
                  ASSERT(RECORD     == cat->recordLevel());
                  ASSERT(PASS       == cat->passLevel());
                  ASSERT(TRIGGER    == cat->triggerLevel());
                  ASSERT(TRIGGERALL == cat->triggerAllLevel());
              }
              else {
                  ASSERT(EXPRECLEVEL     == cat->recordLevel());
                  ASSERT(EXPPASSLEVEL    == cat->passLevel());
                  ASSERT(EXPTRIGLEVEL    == cat->triggerLevel());
                  ASSERT(EXPTRIGALLLEVEL == cat->triggerAllLevel());
              }
          }

          Cat *cX = mLM.lookupCategory("B.C");
          ASSERT(cX);
          if (veryVeryVerbose) {
              P_(cX->categoryName());
              P_(cX->recordLevel());
              P_(cX->passLevel());
              P_(cX->triggerLevel());
              P(cX->triggerAllLevel());
          }

          ball::Logger& LGR = mLM.getLogger();

          if (verbose) cout << "\tTest message buffer and size." << endl;
          {
              char *buffer = LGR.messageBuffer();  ASSERT(buffer);
              ASSERT(MESSAGE_BUFFER_SIZE == LGR.messageBufferSize());
              for (int i = 0; i < MESSAGE_BUFFER_SIZE; ++i) {
                  buffer[i] = (unsigned char) 0xbb;
              }
          }

          if (verbose) cout << "\tTest Threshold Level Values in Order."
                            << endl;

          // Estimate how many records the default buffer can hold.  This is an
          // "white box" estimation as we know the size of the buffer.
          for (int i = 0; i < 100; ++i) {
              // 160 (pass) < severity < 210 (record)
              LGR.logMessage(*cX, 200, F_, L_, "Message");
          }

          // Publish, then get the number of records published as the estimate
          // of buffer capacity.
          LGR.publish();

          const int BUF_CAP = testObserver.numPublishedRecords();
          int publishCount = BUF_CAP;     // # of records published so far
          if (veryVeryVerbose) { P(BUF_CAP); }

          // Verify pass through
          for (int i = 0; i < 100; ++i) {
              // 110 (trigger) < severity < 160 (pass)
              LGR.logMessage(*cX, 150, F_, L_, "Message");
          }

          publishCount += 100;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          // Publish all, verify buffer is published
          mLM.publishAll();
          publishCount += BUF_CAP;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          if (veryVeryVerbose) {
              P(publishCount);
              P(testObserver.numPublishedRecords());
          }

          // Verify 'removeAll' resets buffer
          for (int i = 0; i < BUF_CAP; ++i) {
              // 160 (pass) < severity < 210 (record)
              LGR.logMessage(*cX, 200, F_, L_, "Message");
          }
          LGR.removeAll();

          // Manually publish, no record to publish.
          LGR.publish();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          // Trigger, verify only 2 records are published (pass through and
          // then actually publish)
          LGR.logMessage(*cX, 80, F_, L_, "trigger again");
          publishCount += 2;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          // verify record threshold checking.
          LGR.logMessage(*cX, 255, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 230, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 211, F_, L_, "Ignored Message");
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 210, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 209, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 199, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 161, F_, L_, "Recorded Message");
          publishCount += 4;
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 160, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 159, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 130, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 111, F_, L_, "Pass Through Message");
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 110, F_, L_, "Trigger Message");
          LGR.logMessage(*cX, 109, F_, L_, "Trigger Message");
          LGR.logMessage(*cX, 100, F_, L_, "Trigger Message");
          LGR.logMessage(*cX,  99, F_, L_, "Trigger Message");
          LGR.logMessage(*cX,  61, F_, L_, "Trigger Message");
          publishCount += 10;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 60, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX, 59, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX, 10, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX,  9, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX,  1, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX,  0, F_, L_, "Trigger All Message");
          publishCount += 12;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          if (verbose) cout << "\tTest Threshold Level Values Not in Order."
                            << endl;
          ASSERT(0 == cX->setLevels(150, 160, 100, 50));

          LGR.logMessage(*cX, 255, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 200, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 161, F_, L_, "Ignored Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 160, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 159, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 151, F_, L_, "Pass Through Message");
          publishCount += 3;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 150, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 149, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 110, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 101, F_, L_, "Recorded Message");
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          ASSERT(0 == cX->setLevels(50, 100, 150, 200));

          LGR.logMessage(*cX, 255, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 201, F_, L_, "Ignored Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 200, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX, 199, F_, L_, "Trigger All Message");
          LGR.logMessage(*cX, 151, F_, L_, "Trigger All Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 150, F_, L_, "Trigger Message");
          LGR.logMessage(*cX, 149, F_, L_, "Trigger Message");
          LGR.logMessage(*cX, 101, F_, L_, "Trigger Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 100, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX,  99, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX,  51, F_, L_, "Pass Through Message");
          // each record is published only once -- pass through, since the
          // record is not stored.
          publishCount += 3;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 50, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 49, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 10, F_, L_, "Recorded Message");
          LGR.logMessage(*cX,  9, F_, L_, "Recorded Message");
          LGR.logMessage(*cX,  1, F_, L_, "Recorded Message");
          LGR.logMessage(*cX,  0, F_, L_, "Recorded Message");
          // each record is published twice: pass through and then trigger
          publishCount += 12;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          if (verbose) cout << "\tTest Logging with Two Categories." << endl;

          ASSERT(0 == cX->setLevels(210, 160, 110, 60));

          Cat *cY = mLM.lookupCategory("B");
          // Category "B"'s thresholds: 220/170/120/70 (R/P/T/TA)
          ASSERT(cY);

          // Transfer all threshold levels from the category to a
          // 'ball::ThresholdAggregate' object, and then clear the threshold
          // levels of the category.

          // verify record threshold checking.
          LGR.logMessage(*cX, 255, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 230, F_, L_, "Ignored Message");
          LGR.logMessage(*cX, 211, F_, L_, "Ignored Message");
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cY, 255, F_, L_, "Ignored Message");
          LGR.logMessage(*cY, 230, F_, L_, "Ignored Message");
          LGR.logMessage(*cY, 221, F_, L_, "Ignored Message");

          // verify that records of both categories are published already
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 210, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 209, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 199, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 161, F_, L_, "Recorded Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cY, 220, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 219, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 199, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 171, F_, L_, "Recorded Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          publishCount += 8;

          // Trigger in one category also affects in others
          publishCount += 2;
          LGR.logMessage(*cX, 80, F_, L_, "trigger");
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 160, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 159, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 130, F_, L_, "Pass Through Message");
          LGR.logMessage(*cX, 111, F_, L_, "Pass Through Message");
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cY, 170, F_, L_, "Pass Through Message");
          LGR.logMessage(*cY, 169, F_, L_, "Pass Through Message");
          LGR.logMessage(*cY, 130, F_, L_, "Pass Through Message");
          LGR.logMessage(*cY, 121, F_, L_, "Pass Through Message");
          publishCount += 4;
          ASSERT(publishCount == testObserver.numPublishedRecords());

          publishCount += 8;

          LGR.logMessage(*cX, 110, F_, L_, "Trigger Message");
          publishCount += 2;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cX, 210, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 209, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 199, F_, L_, "Recorded Message");
          LGR.logMessage(*cX, 161, F_, L_, "Recorded Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());

          LGR.logMessage(*cY, 220, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 219, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 199, F_, L_, "Recorded Message");
          LGR.logMessage(*cY, 171, F_, L_, "Recorded Message");
          ASSERT(publishCount == testObserver.numPublishedRecords());
          publishCount += 8;

          LGR.logMessage(*cY, 70, F_, L_, "Trigger All Message");
          publishCount += 2;
          ASSERT(publishCount == testObserver.numPublishedRecords());
          LGR.publish();
          mLM.publishAll();
          ASSERT(publishCount == testObserver.numPublishedRecords());

          if (0 == a) {
              ball::LoggerManager::shutDownSingleton();
          }
          else {
              g_overrideManager = 0;
          }
        }
      } break;
      case 13: {
        // --------------------------------------------------------------------
        // TESTING MANIPULATOR (ball::LoggerCategoryManip)
        //
        // Concerns:
        //   The basic concern is that the constructor, the destructor, the
        //   manipulators:
        //    - void advance();
        //    - Cat& operator()();
        //   and the accessor:
        //    - operator const void *() const;
        //   operate as expected.
        //
        // Plan:
        //   Add categories of various names and threshold level values to the
        //   target logger manager.  Create a manipulator.  Change the
        //   threshold level values using the modifiable access provided by the
        //   manipulator.  Verify the values are changed.  Change the values
        //   back to original values, verify they are reset to original values.
        //
        // Testing:
        //   ball::LoggerCategoryManip(ball::LoggerManager *lm);
        //   ~ball::LoggerCategoryManip();
        //   void advance();
        //   Cat& operator()();
        //   operator const void *() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Test ball::LoggerCategoryManip" << endl
                                  << "=============================" << endl;

        ball::TestObserver               testObserver(cout);
        ball::LoggerManagerConfiguration mLMC;
        ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
        Obj& mLM = Obj::singleton();

        for (int i = 0; i < NUM_NAMES; ++i) {
            // mod 253 so plus 1 is in valid range.
            ASSERT(mLM.addCategory(NAMES[i],
                                   (i + 7)  * (i + 23) % 253,   // record
                                   (i + 3)  * (i + 11) % 253,   // pass
                                   (i + 5)  * (i + 17) % 253,   // trigger
                                   (i + 13) * (i + 29) % 253)); // trigger-all
        }

        int count = 0;
        for (ball::LoggerCategoryManip manip(&mLM); manip; manip.advance()) {
            const Cat *p = mLM.lookupCategory(manip().categoryName());
            const int rl = p->recordLevel();
            const int pl = p->passLevel();
            const int tl = p->triggerLevel();
            const int al = p->triggerAllLevel();
            ASSERT(rl == manip().recordLevel());
            ASSERT(pl == manip().passLevel());
            ASSERT(tl == manip().triggerLevel());
            ASSERT(al == manip().triggerAllLevel());

            manip().setLevels(manip().recordLevel() + 1,
                              manip().passLevel() + 1,
                              manip().triggerLevel() + 1,
                              manip().triggerAllLevel() + 1);

            ASSERT(rl + 1 == p->recordLevel());
            ASSERT(pl + 1 == p->passLevel());
            ASSERT(tl + 1 == p->triggerLevel());
            ASSERT(al + 1 == p->triggerAllLevel());

            manip().setLevels(manip().recordLevel() - 1,
                              manip().passLevel() - 1,
                              manip().triggerLevel() - 1,
                              manip().triggerAllLevel() - 1);

            ASSERT(rl == p->recordLevel());
            ASSERT(pl == p->passLevel());
            ASSERT(tl == p->triggerLevel());
            ASSERT(al == p->triggerAllLevel());

            ++count;
        }
        ASSERT(NUM_NAMES + 1 == count);  // + 1 for the *Default* *Category*

      } break;
      case 12: {
        // --------------------------------------------------------------------
        // TESTING CATEGORY ITERATOR
        //
        // Concerns:
        //   The basic concerns for the iterator are that the constructor,
        //   the destructor, the manipulator 'operator++' and the accessors:
        //    - operator const void *() const;
        //    - const Cat& operator()() const;
        //   operate as expected.
        //
        // Plan:
        // TBD doc
        //   Create a map of 'bsl::string' and 'ball::Category *'; add to the
        //   logger manager various categories with different threshold
        //   levels.  Add to our map the name of the category and the address
        //   of the category obtained when it is added.  Create an iterator of
        //   the logger manager, walk through the categories.  Obtain the name
        //   of the category returned by the iterator, use this name to lookup
        //   in our map, verify that a category is obtained, and verify all
        //   threshold levels are equal.  Remove the map entry from our map.
        //   Verify that when the iteration is complete, our map is empty.
        //   Note that the Default Category always presents in the logger
        //   manager, and need to be added to our map separately.
        //
        // Testing:
        //   ball::LoggerCategoryIter(const ball::LoggerManager& lm);
        //   ~ball::LoggerCategoryIter();
        //   void operator++();
        //   operator const void *() const;
        //   const Cat& operator()() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "TEST ITERATOR" << endl
                                  << "=============" << endl;

        ball::TestObserver               testObserver(cout);
        ball::LoggerManagerConfiguration mLMC;
        ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
        Obj& mLM = Obj::singleton();

        Cat *cat;
        bsl::map<bsl::string, const Cat *> nameCatMap;
        for (int i = 0; i < NUM_NAMES; ++i) {
            ASSERT(cat = mLM.addCategory(
                                    NAMES[i],
                                    (i + 7)  * (i + 23) % 255,   // record
                                    (i + 3)  * (i + 11) % 255,   // pass
                                    (i + 5)  * (i + 17) % 255,   // trigger
                                    (i + 13) * (i + 29) % 255)); // trigger-all
            nameCatMap.insert(bsl::make_pair(bsl::string(NAMES[i]), cat));
        }

        // add *Default* *Category*
        nameCatMap.insert(bsl::make_pair(mLM.defaultCategory().categoryName(),
                                         &mLM.defaultCategory()));

        for (ball::LoggerCategoryIter it(mLM); it; ++it) {
            bsl::map<bsl::string, const Cat *>::iterator itr =
                                        nameCatMap.find(it().categoryName());
            ASSERT(itr != nameCatMap.end());
            const Cat *cat = itr->second;
            ASSERT(cat);
            ASSERT(cat->recordLevel()     == it().recordLevel());
            ASSERT(cat->passLevel()       == it().passLevel());
            ASSERT(cat->triggerLevel()    == it().triggerLevel());
            ASSERT(cat->triggerAllLevel() == it().triggerAllLevel());
            nameCatMap.erase(it().categoryName());
        }
        ASSERT(0 == nameCatMap.size())

      } break;
      case 11: {
        // --------------------------------------------------------------------
        // TESTING PRIMARY MANIPULATORS AND ACCESSORS
        //
        // Concerns:
        //   We want to make sure that we can set various properties of the
        //   logger manager correctly.
        //
        // Plan:
        // TBD doc
        //   Initialize logger manager with name filter and default threshold
        //   levels callback.  Add categories with hierarchical names, verify
        //   the inheritance of threshold values.  Add categories with invalid
        //   threshold values, verify that these add operations return fail
        //   values.  Add a category with no ancestors, verify the default
        //   values are used.  Set the default values to different values, add
        //   another category with no ancestors, verify new default values are
        //   used.  Set each of the threshold levels to all valid values,
        //   verify the threshold levels are set.  Get the *Default*
        //   *Category*; set the threshold levels of the *Default* *Category*
        //   to all valid values; verify they are set.  Reset the *Default*
        //   *Category* threshold levels (to factory default); verify they are
        //   set.  Set the maximum number of categories to a value less than
        //   the current length; verify we can still get the categories already
        //   added, but cannot add new categories.  Set the maximum number to
        //   a value 1 larger then the current number of categories; verify
        //   that we can add no more than one new category.  Set the maximum
        //   number of categories to 0; verify that we can add categories.
        //
        // Testing:
        //   Cat *lookupCategory(const char *name);
        //   Cat *addCategory(const char *name, int, int, int, int);
        //   const Cat *setCategory(const char *name);
        //   Cat *setCategory(const char *name, int, int, int, int);
        //   int setDefaultThresholdLevels(int, int, int, int);
        //   int resetDefaultThresholdLevels();
        //   void setCategoryThresholdsToCurrentDefaults(Cat *cat);
        //   void setCategoryThresholdsToFactoryDefaults(Cat *cat);
        //   void setMaxNumCategories(int);
        //   const Cat *lookupCategory(const char *name) const;
        //   const Cat& defaultCategory() const;
        //   const char *categoryName() const;
        //   int defaultRecordThresholdLevel() const;
        //   int defaultPassThresholdLevel() const;
        //   int defaultTriggerThresholdLevel() const;
        //   int defaultTriggerAllThresholdLevel() const;
        //   int maxNumCategories() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "TEST PRIMARY MANIPULATORS" << endl
                                  << "=========================" << endl;

        ball::TestObserver testObserver(cout);

        Cnf nameFilter(&toLower);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&inheritThresholdLevels);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("FACTORIAL");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "n!");

        ball::Logger::UserPopulatorCallback populator(&myPopulator);

        ball::LoggerManagerConfiguration mLMC;
        mLMC.setCategoryNameFilterCallback(nameFilter);
        mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
        mLMC.setUserFields(schema, populator);

        ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
        Obj& mLM = Obj::singleton();  const Obj& LM = mLM;

        const int dfltRecLevel     = mLM.defaultRecordThresholdLevel();
        const int dfltPassLevel    = mLM.defaultPassThresholdLevel();
        const int dfltTrigLevel    = mLM.defaultTriggerThresholdLevel();
        const int dfltTrigAllLevel = mLM.defaultTriggerAllThresholdLevel();

        Cat       *cat1 = mLM.addCategory("BLOOMBERG.BAE", 64, 48, 32, 16);
        ASSERT(cat1);
        ASSERT(2    == mLM.numCategories());
        ASSERT(cat1 == mLM.lookupCategory("BLOOMBERG.bae"));
        ASSERT( 0   == bsl::strcmp("bloomberg.bae", cat1->categoryName()));
        ASSERT(64   == cat1->recordLevel());
        ASSERT(48   == cat1->passLevel());
        ASSERT(32   == cat1->triggerLevel());
        ASSERT(16   == cat1->triggerAllLevel());

        const Cat *cat2 = mLM.setCategory("bloomberg.bae.bael");
        ASSERT(cat2);
        ASSERT(3    == mLM.numCategories());
        ASSERT(cat2 == mLM.lookupCategory("Bloomberg.Bae.Bael"));
        // verify inheritance of levels
        ASSERT(64 == cat2->recordLevel());
        ASSERT(48 == cat2->passLevel());
        ASSERT(32 == cat2->triggerLevel());
        ASSERT(16 == cat2->triggerAllLevel());

        static const struct {
            int d_line;            // line number
            int d_recordLevel;     // record level
            int d_passLevel;       // pass level
            int d_triggerLevel;    // trigger level
            int d_triggerAllLevel; // trigger all level
        } DATA[] = {
            // line    record    pass     trigger  triggerAll
            // no.     level     level     level     level
            // ----    ------    ------    ------    -----           // ADJUST
            {  L_,     256,      0,        0,        0                      },
            {  L_,     0,        256,      0,        0                      },
            {  L_,     0,        0,        256,      0                      },
            {  L_,     0,        0,        0,        256                    },
            {  L_,     256,      256,      256,      256                    },
            {  L_,     INT_MAX,  0,        0,        0                      },
            {  L_,     0,        INT_MAX,  0,        0                      },
            {  L_,     0,        0,        INT_MAX,  0                      },
            {  L_,     0,        0,        0,        INT_MAX                },
            {  L_,     INT_MAX,  INT_MAX,  INT_MAX,  INT_MAX                },
            {  L_,     -1,       0,        0,        0                      },
            {  L_,     0,        -1,       0,        0                      },
            {  L_,     0,        0,        -1,       0                      },
            {  L_,     0,        0,        0,        -1                     },
            {  L_,     -1,       -1,       -1,       -1                     },
            {  L_,     INT_MIN,  0,        0,        0                      },
            {  L_,     0,        INT_MIN,  0,        0                      },
            {  L_,     0,        0,        INT_MIN,  0                      },
            {  L_,     0,        0,        0,        INT_MIN                },
            {  L_,     INT_MIN,  INT_MIN,  INT_MIN,  INT_MIN                },
        };
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        const char *CAT_NAME = "Bloomberg.Bae.Bael";
        const int   NUM_CAT  = mLM.numCategories();

        for (int i = 0; i < NUM_DATA; ++i) {
            const int LINE             = DATA[i].d_line;
            const int RECORD_LEVEL     = DATA[i].d_recordLevel;
            const int PASS_LEVEL       = DATA[i].d_passLevel;
            const int TRIGGER_LEVEL    = DATA[i].d_triggerLevel;
            const int TRIGGERALL_LEVEL = DATA[i].d_triggerAllLevel;

            if (veryVerbose) {
                P_(LINE); P_(RECORD_LEVEL); P_(PASS_LEVEL);
                P_(TRIGGER_LEVEL); P(TRIGGERALL_LEVEL);
            }
            const Cat *cat = mLM.setCategory(CAT_NAME,
                                             RECORD_LEVEL,
                                             PASS_LEVEL,
                                             TRIGGER_LEVEL,
                                             TRIGGERALL_LEVEL);
            LOOP_ASSERT(LINE, 0       == cat);
            LOOP_ASSERT(LINE, NUM_CAT == mLM.numCategories());
        }

        ASSERT(cat2 == mLM.setCategory(CAT_NAME, 40, 30, 20, 10));
        ASSERT(cat2);
        ASSERT(40 == cat2->recordLevel());
        ASSERT(30 == cat2->passLevel());
        ASSERT(20 == cat2->triggerLevel());
        ASSERT(10 == cat2->triggerAllLevel());

        const Cat *cat6 = mLM.setCategory("Bloomberg.Bae.Bael.logging",
                                         80, 60, 40, 20);
        ASSERT(cat6);
        ASSERT(0  == (cat2 == cat6));
        ASSERT(80 == cat6->recordLevel());
        ASSERT(60 == cat6->passLevel());
        ASSERT(40 == cat6->triggerLevel());
        ASSERT(20 == cat6->triggerAllLevel());

        const Cat *cat3 = mLM.setCategory("Equities.Graphics");
        ASSERT(dfltRecLevel     == cat3->recordLevel());
        ASSERT(dfltPassLevel    == cat3->passLevel());
        ASSERT(dfltTrigLevel    == cat3->triggerLevel());
        ASSERT(dfltTrigAllLevel == cat3->triggerAllLevel());

        if (veryVeryVerbose) {
            P_(dfltRecLevel);
            P_(dfltPassLevel);
            P_(dfltTrigLevel);
            P(dfltTrigAllLevel);
        }

        mLM.setDefaultThresholdLevels(192, 144, 96, 48);
        const int dfltRecLevel2     = mLM.defaultRecordThresholdLevel();
        const int dfltPassLevel2    = mLM.defaultPassThresholdLevel();
        const int dfltTrigLevel2    = mLM.defaultTriggerThresholdLevel();
        const int dfltTrigAllLevel2 = mLM.defaultTriggerAllThresholdLevel();
        ASSERT(192 == dfltRecLevel2);
        ASSERT(144 == dfltPassLevel2);
        ASSERT( 96 == dfltTrigLevel2);
        ASSERT( 48 == dfltTrigAllLevel2);

        if (veryVeryVerbose) {
            P_(dfltRecLevel2);
            P_(dfltPassLevel2);
            P_(dfltTrigLevel2);
            P(dfltTrigAllLevel2);
        }

        const Cat *cat4 = mLM.setCategory("Equities.Graphics.G");
        ASSERT(cat4);
        ASSERT(dfltRecLevel     == cat4->recordLevel());
        ASSERT(dfltPassLevel    == cat4->passLevel());
        ASSERT(dfltTrigLevel    == cat4->triggerLevel());
        ASSERT(dfltTrigAllLevel == cat4->triggerAllLevel());

        Cat *cat4a = mLM.lookupCategory("Equities.Graphics.G");
        ASSERT(cat4 == cat4a);
        mLM.setCategoryThresholdsToCurrentDefaults(cat4a);
        ASSERT(dfltRecLevel2     == cat4a->recordLevel());
        ASSERT(dfltPassLevel2    == cat4a->passLevel());
        ASSERT(dfltTrigLevel2    == cat4a->triggerLevel());
        ASSERT(dfltTrigAllLevel2 == cat4a->triggerAllLevel());

        mLM.setCategoryThresholdsToFactoryDefaults(cat4a);
        ASSERT(dfltRecLevel     == cat4a->recordLevel());
        ASSERT(dfltPassLevel    == cat4a->passLevel());
        ASSERT(dfltTrigLevel    == cat4a->triggerLevel());
        ASSERT(dfltTrigAllLevel == cat4a->triggerAllLevel());

        const Cat *cat5 = mLM.setCategory("Equities");
        ASSERT(cat5);
        ASSERT(192 == cat5->recordLevel());
        ASSERT(144 == cat5->passLevel());
        ASSERT( 96 == cat5->triggerLevel());
        ASSERT( 48 == cat5->triggerAllLevel());

        mLM.resetDefaultThresholdLevels();
        ASSERT(dfltRecLevel     == mLM.defaultRecordThresholdLevel());
        ASSERT(dfltPassLevel    == mLM.defaultPassThresholdLevel());
        ASSERT(dfltTrigLevel    == mLM.defaultTriggerThresholdLevel());
        ASSERT(dfltTrigAllLevel == mLM.defaultTriggerAllThresholdLevel());

        Cat *cat7 = mLM.addCategory("test_category", 0, 0, 0, 0);
        ASSERT(cat7);
        ASSERT(0 == cat7->recordLevel());
        ASSERT(0 == cat7->passLevel());
        ASSERT(0 == cat7->triggerLevel());
        ASSERT(0 == cat7->triggerAllLevel());

        const Cat& dfltCategory = mLM.defaultCategory();
        const int dfltCat_recLevel     = dfltCategory.recordLevel();
        const int dfltCat_passLevel    = dfltCategory.passLevel();
        const int dfltCat_trigLevel    = dfltCategory.triggerLevel();
        const int dfltCat_trigAllLevel = dfltCategory.triggerAllLevel();
        ASSERT(&dfltCategory == LM.lookupCategory(DEFAULT_CATEGORY_NAME));

        for (int i = 0; i < 256; ++i) {
            ASSERT(0 == cat7->setLevels(i, 0, 0, 0));
            ASSERT(i == cat7->recordLevel());
            ASSERT(0 == cat7->passLevel());
            ASSERT(0 == cat7->triggerLevel());
            ASSERT(0 == cat7->triggerAllLevel());

            ASSERT(0 == mLM.setDefaultThresholdLevels(i, 0, 0, 0));
            ASSERT(i == mLM.defaultRecordThresholdLevel());
            ASSERT(0 == mLM.defaultPassThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerAllThresholdLevel());
            ASSERT(dfltCat_recLevel == dfltCategory.recordLevel());
        }

        for (int i = 0; i < 256; ++i) {
            ASSERT(0 == cat7->setLevels(0, i, 0, 0));
            ASSERT(0 == cat7->recordLevel());
            ASSERT(i == cat7->passLevel());
            ASSERT(0 == cat7->triggerLevel());
            ASSERT(0 == cat7->triggerAllLevel());

            ASSERT(0 == mLM.setDefaultThresholdLevels(0, i, 0, 0));
            ASSERT(0 == mLM.defaultRecordThresholdLevel());
            ASSERT(i == mLM.defaultPassThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerAllThresholdLevel());
            ASSERT(dfltCat_passLevel == dfltCategory.passLevel());
        }

        for (int i = 0; i < 256; ++i) {
            ASSERT(0 == cat7->setLevels(0, 0, i, 0));
            ASSERT(0 == cat7->recordLevel());
            ASSERT(0 == cat7->passLevel());
            ASSERT(i == cat7->triggerLevel());
            ASSERT(0 == cat7->triggerAllLevel());

            ASSERT(0 == mLM.setDefaultThresholdLevels(0, 0, i, 0));
            ASSERT(0 == mLM.defaultRecordThresholdLevel());
            ASSERT(0 == mLM.defaultPassThresholdLevel());
            ASSERT(i == mLM.defaultTriggerThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerAllThresholdLevel());
            ASSERT(dfltCat_trigLevel == dfltCategory.triggerLevel());
        }

        for (int i = 0; i < 256; ++i) {
            ASSERT(0 == cat7->setLevels(0, 0, 0, i));
            ASSERT(0 == cat7->recordLevel());
            ASSERT(0 == cat7->passLevel());
            ASSERT(0 == cat7->triggerLevel());
            ASSERT(i == cat7->triggerAllLevel());

            ASSERT(0 == mLM.setDefaultThresholdLevels(0, 0, 0, i));
            ASSERT(0 == mLM.defaultRecordThresholdLevel());
            ASSERT(0 == mLM.defaultPassThresholdLevel());
            ASSERT(0 == mLM.defaultTriggerThresholdLevel());
            ASSERT(i == mLM.defaultTriggerAllThresholdLevel());
            ASSERT(dfltCat_trigAllLevel == dfltCategory.triggerAllLevel());
        }

                                     ASSERT(0 == mLM.maxNumCategories());
        mLM.setMaxNumCategories(1);  ASSERT(1 == mLM.maxNumCategories());

        // verify that we can still get existing categories...
        ASSERT(mLM.lookupCategory("test_category"));
        ASSERT(mLM.lookupCategory("Equities"));
        ASSERT(mLM.lookupCategory("Equities.Graphics"));
        ASSERT(mLM.lookupCategory("Equities.Graphics.G"));
        ASSERT(mLM.lookupCategory("Bloomberg.Bae.Bael"));
        ASSERT(mLM.lookupCategory("Bloomberg.Bae.Bael.Logging"));
        ASSERT(mLM.lookupCategory("Bloomberg.Bae"));

        // ...but cannot add new categories
        ASSERT(0 == mLM.addCategory("one_more_category", 0, 0, 0, 0));

        mLM.setMaxNumCategories(9);  // currently have 7 + *Default* *Category*
        ASSERT(mLM.addCategory("one_more_category", 0, 0, 0, 0));

        ASSERT(0 == mLM.addCategory("and_another", 0, 0, 0, 0));
        mLM.setMaxNumCategories(0);
        ASSERT(mLM.addCategory("and_another", 0, 0, 0, 0));

      } break;
      case 10: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON PRIMARY
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   ball::LoggerManager(Obs*, ball::LoggerManagerConfiguration&, *ba);
        //   static bool isInitialized()
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing 'initSingleton'" << endl
                                  << "=======================" << endl;

        bslma::TestAllocator defaultTestAllocator(veryVeryVerbose);
        bslma::TestAllocator globalTestAllocator(veryVeryVerbose);
        bslma::TestAllocator objectTestAllocator(veryVeryVerbose);

        bslma::TestAllocator *DA = &defaultTestAllocator;
        bslma::TestAllocator *GA = &globalTestAllocator;
        bslma::TestAllocator *OA = &objectTestAllocator;

        bslma::DefaultAllocatorGuard taGuard(DA);
        bslma::Default::setGlobalAllocator(GA);

        const bsls::Types::Int64 NUM_BLOCKS_DFLT_ALLOC = DA->numBlocksInUse();
        const bsls::Types::Int64 NUM_BLOCKS_GLOB_ALLOC = GA->numBlocksInUse();
        {
            ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);

            ball::TestObserver testObserver(bsl::cout, OA);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            Cnf nameFilter(&toLower, OA);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&simpleThresholdLevels, OA);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            bdlmxxx::Schema schema(OA);
            bdlmxxx::RecordDef *p = schema.createRecord("initSingleton");
            p->appendField(bdlmxxx::ElemType::BDEM_INT, "int");
            p->appendField(bdlmxxx::ElemType::BDEM_STRING, "string");
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ball::Logger::UserPopulatorCallback populator(&simplePopulator, OA);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            ball::LoggerManagerDefaults mLMD;
            mLMD.setDefaultLoggerBufferSizeIfValid(2048);
            mLMD.setDefaultThresholdLevelsIfValid(19, 17, 13, 11);

            ball::LoggerManagerConfiguration mLMC(OA);
            mLMC.setDefaultValues(mLMD);
            mLMC.setUserFields(schema, populator);
            mLMC.setCategoryNameFilterCallback(nameFilter);
            mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());

            if (veryVerbose) { cout << "mLMC: "; mLMC.print(cout, -1); }

            ASSERT(0 == Obj::isInitialized());
            Obj::initSingleton(&testObserver, mLMC);
            ASSERT(1 == Obj::isInitialized());
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(NUM_BLOCKS_GLOB_ALLOC <  GA->numBlocksInUse());

            Obj& mLM = Obj::singleton();

            ASSERT(&testObserver == mLM.observer());
            ASSERT(1             == mLM.numCategories());

            const Cat& dfltCat = mLM.defaultCategory();
            ASSERT(0  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                     dfltCat.categoryName()));
            ASSERT(19 == dfltCat.recordLevel());
            ASSERT(17 == dfltCat.passLevel());
            ASSERT(13 == dfltCat.triggerLevel());
            ASSERT(11 == dfltCat.triggerAllLevel());

            const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(cat);
            ASSERT(cat == mLM.lookupCategory("BLOOMBERG.bae"));
            ASSERT( 0  == bsl::strcmp("bloomberg.bae", cat->categoryName()));
            ASSERT(64  == cat->recordLevel());
            ASSERT(48  == cat->passLevel());
            ASSERT(32  == cat->triggerLevel());
            ASSERT(16  == cat->triggerAllLevel());

            const Cat *scat = mLM.setCategory("setCategory(cont char *)");
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(scat);
            ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
            ASSERT( 0   == bsl::strcmp("setcategory(cont char *)",
                                       scat->categoryName()));
            ASSERT(44   == scat->recordLevel());
            ASSERT(33   == scat->passLevel());
            ASSERT(22   == scat->triggerLevel());
            ASSERT(11   == scat->triggerAllLevel());

            // log a test message

            if (veryVerbose) testObserver.setVerbose(1);
            ball::Logger& lgr = mLM.getLogger();
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(2048 == lgr.messageBufferSize());
            ASSERT(   0 == testObserver.numPublishedRecords());
            const char *MESSAGE = "initSingleton variant 16";
            const int   LINE    = L_ + 1;
            lgr.logMessage(dfltCat, 17, __FILE__, LINE, MESSAGE);
            ASSERT(NUM_BLOCKS_DFLT_ALLOC == DA->numBlocksInUse());
            ASSERT(1 == testObserver.numPublishedRecords());
            const Rec&  R = testObserver.lastPublishedRecord();
            if (veryVerbose) cout << R << endl;

            const Attr& A = R.fixedFields();
            ASSERT(0    == bsl::strcmp(DEFAULT_CATEGORY_NAME, A.category()));
            ASSERT(17   == A.severity());
            ASSERT(0    == bsl::strcmp(__FILE__, A.fileName()));
            ASSERT(LINE == A.lineNumber());
            ASSERT(0    == bsl::strcmp(MESSAGE, A.message()));

            const List& L = R.userFields();
            ASSERT(   2 == L.length());
            ASSERT(1066 == L.theInt(0));
            ASSERT(   0 == bsl::strcmp("XXX", L.theString(1).c_str()));

            ASSERT(1 == Obj::isInitialized());
            Obj::shutDownSingleton();
            ASSERT(0 == Obj::isInitialized());

            ASSERT(0 == NUM_BLOCKS_GLOB_ALLOC);
        }

      } break;
      case 9: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 14
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, Dtc&, Fdt&, Sch&, Pop&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 14" << endl
                         << "==================================" << endl;

        if (verbose)
            cout << endl
                 << "\t'initSingleton(Obs*, Dtc&, Fdt&, Sch&, Pop&, *ba = 0)'"
                 << endl;

        ball::TestObserver testObserver(cout);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&simpleThresholdLevels);

        Obj::FactoryDefaultThresholds fdt(19, 17, 13, 11);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("initSingleton");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "int");
        p->appendField(bdlmxxx::ElemType::BDEM_STRING, "string");

        ball::Logger::UserPopulatorCallback populator(&simplePopulator);

        Obj::initSingleton(&testObserver,
                           thresholdsCallback,
                           fdt,
                           schema,
                           populator);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                 dfltCat.categoryName()));
        ASSERT(19 == dfltCat.recordLevel());
        ASSERT(17 == dfltCat.passLevel());
        ASSERT(13 == dfltCat.triggerLevel());
        ASSERT(11 == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(44   == scat->recordLevel());
        ASSERT(33   == scat->passLevel());
        ASSERT(22   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 14";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, 17, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0    == bsl::strcmp(DEFAULT_CATEGORY_NAME, A.category()));
        ASSERT(17   == A.severity());
        ASSERT(0    == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE == A.lineNumber());
        ASSERT(0    == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(   2 == L.length());
        ASSERT(1066 == L.theInt(0));
        ASSERT(   0 == bsl::strcmp("XXX", L.theString(1).c_str()));

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 8: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 13
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, Dtc&, Sch&, Pop&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 13" << endl
                         << "==================================" << endl;

        if (verbose)
            cout << endl
                 << "\t'initSingleton(Obs*, Dtc&, Sch&, Pop&, *ba = 0)'"
                 << endl;

        ball::TestObserver testObserver(cout);

        Obj::DefaultThresholdLevelsCallback
            thresholdsCallback(&simpleThresholdLevels);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("initSingleton");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "int");
        p->appendField(bdlmxxx::ElemType::BDEM_STRING, "string");

        ball::Logger::UserPopulatorCallback populator(&simplePopulator);

        Obj::initSingleton(&testObserver,
                           thresholdsCallback,
                           schema,
                           populator);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0                  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                                 dfltCat.categoryName()));
        ASSERT(FACTORY_RECORD     == dfltCat.recordLevel());
        ASSERT(FACTORY_PASS       == dfltCat.passLevel());
        ASSERT(FACTORY_TRIGGER    == dfltCat.triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(44   == scat->recordLevel());
        ASSERT(33   == scat->passLevel());
        ASSERT(22   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 13";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, FACTORY_PASS, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0            == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                           A.category()));
        ASSERT(FACTORY_PASS == A.severity());
        ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE         == A.lineNumber());
        ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(   2 == L.length());
        ASSERT(1066 == L.theInt(0));
        ASSERT(   0 == bsl::strcmp("XXX", L.theString(1).c_str()));

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 7: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 10
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, Fdt&, Sch&, Pop&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 10" << endl
                         << "==================================" << endl;

        if (verbose)
            cout << endl
                 << "\t'initSingleton(Obs*, Fdt&, Sch&, Pop&, *ba = 0)'"
                 << endl;

        ball::TestObserver testObserver(cout);

        Obj::FactoryDefaultThresholds fdt(19, 17, 13, 11);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("initSingleton");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "int");
        p->appendField(bdlmxxx::ElemType::BDEM_STRING, "string");

        ball::Logger::UserPopulatorCallback populator(&simplePopulator);

        Obj::initSingleton(&testObserver, fdt, schema, populator);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                 dfltCat.categoryName()));
        ASSERT(19 == dfltCat.recordLevel());
        ASSERT(17 == dfltCat.passLevel());
        ASSERT(13 == dfltCat.triggerLevel());
        ASSERT(11 == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(19   == scat->recordLevel());
        ASSERT(17   == scat->passLevel());
        ASSERT(13   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 10";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, 17, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0    == bsl::strcmp(DEFAULT_CATEGORY_NAME, A.category()));
        ASSERT(17   == A.severity());
        ASSERT(0    == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE == A.lineNumber());
        ASSERT(0    == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(   2 == L.length());
        ASSERT(1066 == L.theInt(0));
        ASSERT(   0 == bsl::strcmp("XXX", L.theString(1).c_str()));

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 6: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 9
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, Sch&, Pop&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 9" << endl
                         << "=================================" << endl;

        if (verbose)
            cout << endl << "\t'initSingleton(Obs*, Sch&, Pop&, *ba = 0)'"
                 << endl;

        ball::TestObserver testObserver(cout);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("initSingleton");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "int");
        p->appendField(bdlmxxx::ElemType::BDEM_STRING, "string");

        ball::Logger::UserPopulatorCallback populator(&simplePopulator);

        Obj::initSingleton(&testObserver, schema, populator);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0 == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                dfltCat.categoryName()));
        ASSERT(FACTORY_RECORD     == dfltCat.recordLevel());
        ASSERT(FACTORY_PASS       == dfltCat.passLevel());
        ASSERT(FACTORY_TRIGGER    == dfltCat.triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(FACTORY_RECORD     == scat->recordLevel());
        ASSERT(FACTORY_PASS       == scat->passLevel());
        ASSERT(FACTORY_TRIGGER    == scat->triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == scat->triggerAllLevel());

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 9";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, FACTORY_PASS, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0            == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                           A.category()));
        ASSERT(FACTORY_PASS == A.severity());
        ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE         == A.lineNumber());
        ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(   2 == L.length());
        ASSERT(1066 == L.theInt(0));
        ASSERT(   0 == bsl::strcmp("XXX", L.theString(1).c_str()));

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 5: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 6
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.  Add categories with names
        //   forming hierarchies, verify that whenever an ancestor is present,
        //   the threshold values are inherited, otherwise, appropriate default
        //   values are used.
        //
        // Testing:
        //   initSingleton(Obs*, Dtc&, Fdt&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 6" << endl
                         << "=================================" << endl;

        if (verbose)
            cout << endl << "\t'initSingleton(Obs*, Dtc&, Fdt&, *ba = 0)'"
                 << endl;

        ball::TestObserver testObserver(cout);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&simpleThresholdLevels);

        Obj::FactoryDefaultThresholds fdt(19, 17, 13, 11);

        Obj::initSingleton(&testObserver, thresholdsCallback, fdt);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                 dfltCat.categoryName()));
        ASSERT(19 == dfltCat.recordLevel());
        ASSERT(17 == dfltCat.passLevel());
        ASSERT(13 == dfltCat.triggerLevel());
        ASSERT(11 == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(44   == scat->recordLevel());
        ASSERT(33   == scat->passLevel());
        ASSERT(22   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        thresholdsCallback = &inheritThresholdLevels;
        mLM.setDefaultThresholdLevelsCallback(&thresholdsCallback);

        // Verify that the threshold level values are inherited from the
        // "parent" category of the added category, or default values if none
        // exists, according to our defined callback.  Note that there is no
        // name filter installed.

        if (verbose) cout << endl << "\tTest New Category Threshold Levels"
                                  << " with Inheritance" << endl;
        static const struct {
            const char *d_catName;         // category name
            int         d_recLevel;        // record level (dflt, if 0)
            int         d_passLevel;       // pass level (dflt, if 0)
            int         d_trigLevel;       // trigger level (dflt, if 0)
            int         d_trigAllLevel;    // trigger-all level (dflt, if 0)
            int         d_expRecLevel;     // expected record level
            int         d_expPassLevel;    // expected pass level
            int         d_expTrigLevel;    // expected trigger level
            int         d_expTrigAllLevel; // expected trigger-all level
        } DATA[] = {
// category       input   input  input  input   exp'd  exp'd   exp'd  exp'd
//   name         rec l   ps l   tg l   tgA l   rec l  ps l    tg l   tgA l
// --------       -----   -----  -----  -----   -----  -----   -----  -----
{ "A",             200,    150,   100,   50,     200,   150,    100,   50    },
{ "a.B",             0,      0,     0,    0,       0,     0,      0,    0    },
{ "A.B.C.D",         0,      0,     0,    0,     200,   150,    100,   50    },
{ "B.C",           210,    160,   110,   60,     210,   160,    110,   60    },
{ "B",             220,    170,   120,   70,     220,   170,    120,   70    },
{ "B.C.A",           0,      0,     0,    0,     210,   160,    110,   60    },
{ "B.c.A",           0,      0,     0,    0,     220,   170,    120,   70    },
{ "B.A.C",           0,      0,     0,    0,     220,   170,    120,   70    },
{ "AA",              0,      0,     0,    0,       0,     0,      0,    0    },
{ "BB",              0,      0,     0,    0,       0,     0,      0,    0    },
{ "Aa.BB",           0,      0,     0,    0,       0,     0,      0,    0    },
{ "A.B.C.D.E.F.G",   0,      0,     0,    0,     200,   150,    100,   50    },
        };
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        for (int i = 0; i < NUM_DATA; ++i) {
            const char *CAT_NAME = DATA[i].d_catName;
            const int expRecLevel     = DATA[i].d_expRecLevel == 0
                                        ? mLM.defaultRecordThresholdLevel()
                                        : DATA[i].d_expRecLevel;
            const int expPassLevel    = DATA[i].d_expPassLevel == 0
                                        ? mLM.defaultPassThresholdLevel()
                                        : DATA[i].d_expPassLevel;
            const int expTrigLevel    = DATA[i].d_expTrigLevel == 0
                                        ? mLM.defaultTriggerThresholdLevel()
                                        : DATA[i].d_expTrigLevel;
            const int expTrigAllLevel = DATA[i].d_expTrigAllLevel == 0
                                        ? mLM.defaultTriggerAllThresholdLevel()
                                        : DATA[i].d_expTrigAllLevel;

            const Cat *cat = DATA[i].d_recLevel
                             ? mLM.addCategory(CAT_NAME,
                                               DATA[i].d_recLevel,
                                               DATA[i].d_passLevel,
                                               DATA[i].d_trigLevel,
                                               DATA[i].d_trigAllLevel)
                             : mLM.setCategory(CAT_NAME);

            if (veryVeryVerbose) {
                P(CAT_NAME);
                P_(cat->recordLevel());
                P_(cat->passLevel());
                P_(cat->triggerLevel());
                P(cat->triggerAllLevel());
                P_(expRecLevel);
                P_(expPassLevel);
                P_(expTrigLevel);
                P(expTrigAllLevel);
            }

            LOOP_ASSERT(i, expRecLevel     == cat->recordLevel());
            LOOP_ASSERT(i, expPassLevel    == cat->passLevel());
            LOOP_ASSERT(i, expTrigLevel    == cat->triggerLevel());
            LOOP_ASSERT(i, expTrigAllLevel == cat->triggerAllLevel());
        }

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 6";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, 17, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0    == bsl::strcmp(DEFAULT_CATEGORY_NAME, A.category()));
        ASSERT(17   == A.severity());
        ASSERT(0    == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE == A.lineNumber());
        ASSERT(0    == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(0 == L.length());

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 4: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 5
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.  Add categories with names
        //   forming hierarchies, verify that whenever an ancestor is present,
        //   the threshold values are inherited, otherwise, appropriate default
        //   values are used.
        //
        // Testing:
        //   initSingleton(Obs*, Dtc&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 5" << endl
                         << "=================================" << endl;

        if (verbose)
            cout << endl << "\t'initSingleton(Obs*, Dtc&, *ba = 0)'" << endl;

        ball::TestObserver testObserver(cout);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&simpleThresholdLevels);

        Obj::initSingleton(&testObserver, thresholdsCallback);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0 == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                dfltCat.categoryName()));
        ASSERT(FACTORY_RECORD     == dfltCat.recordLevel());
        ASSERT(FACTORY_PASS       == dfltCat.passLevel());
        ASSERT(FACTORY_TRIGGER    == dfltCat.triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(44   == scat->recordLevel());
        ASSERT(33   == scat->passLevel());
        ASSERT(22   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        thresholdsCallback = &inheritThresholdLevels;
        mLM.setDefaultThresholdLevelsCallback(&thresholdsCallback);

        // Verify that the threshold level values are inherited from the
        // "parent" category of the added category, or default values if none
        // exists, according to our defined callback.  Note that there is no
        // name filter installed.

        if (verbose) cout << endl << "\tTest New Category Threshold Levels"
                                  << " with Inheritance" << endl;
        static const struct {
            const char *d_catName;         // category name
            int         d_recLevel;        // record level (dflt, if 0)
            int         d_passLevel;       // pass level (dflt, if 0)
            int         d_trigLevel;       // trigger level (dflt, if 0)
            int         d_trigAllLevel;    // trigger-all level (dflt, if 0)
            int         d_expRecLevel;     // expected record level
            int         d_expPassLevel;    // expected pass level
            int         d_expTrigLevel;    // expected trigger level
            int         d_expTrigAllLevel; // expected trigger-all level
        } DATA[] = {
// category       input   input  input  input   exp'd  exp'd   exp'd  exp'd
//   name         rec l   ps l   tg l   tgA l   rec l  ps l    tg l   tgA l
// --------       -----   -----  -----  -----   -----  -----   -----  -----
{ "A",             200,    150,   100,   50,     200,   150,    100,   50    },
{ "a.B",             0,      0,     0,    0,       0,     0,      0,    0    },
{ "A.B.C.D",         0,      0,     0,    0,     200,   150,    100,   50    },
{ "B.C",           210,    160,   110,   60,     210,   160,    110,   60    },
{ "B",             220,    170,   120,   70,     220,   170,    120,   70    },
{ "B.C.A",           0,      0,     0,    0,     210,   160,    110,   60    },
{ "B.c.A",           0,      0,     0,    0,     220,   170,    120,   70    },
{ "B.A.C",           0,      0,     0,    0,     220,   170,    120,   70    },
{ "AA",              0,      0,     0,    0,       0,     0,      0,    0    },
{ "BB",              0,      0,     0,    0,       0,     0,      0,    0    },
{ "Aa.BB",           0,      0,     0,    0,       0,     0,      0,    0    },
{ "A.B.C.D.E.F.G",   0,      0,     0,    0,     200,   150,    100,   50    },
        };
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        for (int i = 0; i < NUM_DATA; ++i) {
            const char *CAT_NAME = DATA[i].d_catName;
            const int expRecLevel     = DATA[i].d_expRecLevel == 0
                                        ? mLM.defaultRecordThresholdLevel()
                                        : DATA[i].d_expRecLevel;
            const int expPassLevel    = DATA[i].d_expPassLevel == 0
                                        ? mLM.defaultPassThresholdLevel()
                                        : DATA[i].d_expPassLevel;
            const int expTrigLevel    = DATA[i].d_expTrigLevel == 0
                                        ? mLM.defaultTriggerThresholdLevel()
                                        : DATA[i].d_expTrigLevel;
            const int expTrigAllLevel = DATA[i].d_expTrigAllLevel == 0
                                        ? mLM.defaultTriggerAllThresholdLevel()
                                        : DATA[i].d_expTrigAllLevel;

            const Cat *cat = DATA[i].d_recLevel
                             ? mLM.addCategory(CAT_NAME,
                                               DATA[i].d_recLevel,
                                               DATA[i].d_passLevel,
                                               DATA[i].d_trigLevel,
                                               DATA[i].d_trigAllLevel)
                             : mLM.setCategory(CAT_NAME);

            if (veryVeryVerbose) {
                P(CAT_NAME);
                P_(cat->recordLevel());
                P_(cat->passLevel());
                P_(cat->triggerLevel());
                P(cat->triggerAllLevel());
                P_(expRecLevel);
                P_(expPassLevel);
                P_(expTrigLevel);
                P(expTrigAllLevel);
            }

            LOOP_ASSERT(i, expRecLevel     == cat->recordLevel());
            LOOP_ASSERT(i, expPassLevel    == cat->passLevel());
            LOOP_ASSERT(i, expTrigLevel    == cat->triggerLevel());
            LOOP_ASSERT(i, expTrigAllLevel == cat->triggerAllLevel());
        }

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 5";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, FACTORY_PASS, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0            == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                           A.category()));
        ASSERT(FACTORY_PASS == A.severity());
        ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE         == A.lineNumber());
        ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(0 == L.length());

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 2
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, Fdt&, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 2" << endl
                         << "=================================" << endl;

        if (verbose)
            cout << endl << "\t'initSingleton(Obs*, Fdt&, *ba = 0)'" << endl;

        ball::TestObserver testObserver(cout);

        Obj::FactoryDefaultThresholds fdt(19, 17, 13, 11);

        Obj::initSingleton(&testObserver, fdt);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0  == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                 dfltCat.categoryName()));
        ASSERT(19 == dfltCat.recordLevel());
        ASSERT(17 == dfltCat.passLevel());
        ASSERT(13 == dfltCat.triggerLevel());
        ASSERT(11 == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT( 0   == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(19   == scat->recordLevel());
        ASSERT(17   == scat->passLevel());
        ASSERT(13   == scat->triggerLevel());
        ASSERT(11   == scat->triggerAllLevel());

        if (verbose) cout << endl << "\tTest New Category Names with No Name"
                          << " filter" << endl;
        // Verify that when no name filter is registered, category names are
        // added unchanged.

        for (int i = 0; i < NUM_NAMES; ++i) {
            const Cat *cat = mLM.setCategory(NAMES[i]);
            ASSERT(cat);
            ASSERT(0 == bsl::strcmp(NAMES[i], cat->categoryName()));
        }

        if (verbose) cout << endl << "\tTest New Category Threshold Levels"
                          << " with No Inheritance" << endl;
        // Verify that when no threshold-level-values callback is installed,
        // all new categories take the default threshold values if they are
        // not specified.
        const char *HIERNAMES[] = {
            "A",
            "A.B",
            "A.B.C.D",
            "B.C",
            "B",
            "B.C.A",
            "B.A.C",
            "AA",
            "BB",
            "Aa.BB",
            "A.B.C.D.E.F.G"
        };
        const int NUM_HIERNAMES = sizeof HIERNAMES / sizeof *HIERNAMES;

        const int recLevel     = mLM.defaultRecordThresholdLevel();
        const int passLevel    = mLM.defaultPassThresholdLevel();
        const int trigLevel    = mLM.defaultTriggerThresholdLevel();
        const int trigAllLevel = mLM.defaultTriggerAllThresholdLevel();
        ASSERT(19 == recLevel);
        ASSERT(17 == passLevel);
        ASSERT(13 == trigLevel);
        ASSERT(11 == trigAllLevel);
        for (int i = 0; i < NUM_HIERNAMES; ++i) {
            const char *NAME = HIERNAMES[i];
            const Cat  *cat  = mLM.setCategory(NAME);
            ASSERT(cat);
            LOOP_ASSERT(i, 0            == bsl::strcmp(NAME,
                                                       cat->categoryName()));
            LOOP_ASSERT(i, recLevel     == cat->recordLevel());
            LOOP_ASSERT(i, passLevel    == cat->passLevel());
            LOOP_ASSERT(i, trigLevel    == cat->triggerLevel());
            LOOP_ASSERT(i, trigAllLevel == cat->triggerAllLevel());
        }

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 2";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, 17, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0    == bsl::strcmp(DEFAULT_CATEGORY_NAME, A.category()));
        ASSERT(17   == A.severity());
        ASSERT(0    == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE == A.lineNumber());
        ASSERT(0    == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(0 == L.length());

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING INITSINGLETON VARIANT 1
        //
        // Concerns:
        //   We want to make sure that we get a valid singleton and that it is
        //   initialized properly, and remains valid as long as we need it, and
        //   we get access to the singleton object correctly.
        //
        // Plan:
        //   We construct a singleton logger manager, make sure the parameters
        //   passed in are correctly hooked up.  We also try to use the
        //   singleton in a destructor of a static object that is defined prior
        //   to the initialization of the singleton.
        //
        // Testing:
        //   initSingleton(Obs*, *ba = 0);
        //   static ball::LoggerManager& singleton();
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "Testing 'initSingleton' variant 1" << endl
                         << "=================================" << endl;

        if (verbose) cout << endl << "\t'initSingleton(Obs*, *ba = 0)'"
                          << endl;

        ball::TestObserver testObserver(cout);

        Obj::initSingleton(&testObserver);
        Obj& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(0 == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                dfltCat.categoryName()));
        ASSERT(FACTORY_RECORD     == dfltCat.recordLevel());
        ASSERT(FACTORY_PASS       == dfltCat.passLevel());
        ASSERT(FACTORY_TRIGGER    == dfltCat.triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == dfltCat.triggerAllLevel());

        const Cat *cat = mLM.addCategory("Bloomberg.Bae", 64, 48, 32, 16);
        ASSERT(cat);
        ASSERT(cat == mLM.lookupCategory("Bloomberg.Bae"));
        ASSERT( 0  == bsl::strcmp("Bloomberg.Bae", cat->categoryName()));
        ASSERT(64  == cat->recordLevel());
        ASSERT(48  == cat->passLevel());
        ASSERT(32  == cat->triggerLevel());
        ASSERT(16  == cat->triggerAllLevel());

        const Cat *scat = mLM.setCategory("setCategory(cont char *)");
        ASSERT(scat);
        ASSERT(scat == mLM.lookupCategory("setCategory(cont char *)"));
        ASSERT(   0 == bsl::strcmp("setCategory(cont char *)",
                                   scat->categoryName()));
        ASSERT(FACTORY_RECORD     == scat->recordLevel());
        ASSERT(FACTORY_PASS       == scat->passLevel());
        ASSERT(FACTORY_TRIGGER    == scat->triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == scat->triggerAllLevel());

        if (verbose) cout << endl << "\tTest New Category Names with No Name"
                                  << " filter" << endl;
        // Verify that when no name filter is registered, category names are
        // added unchanged.

        for (int i = 0; i < NUM_NAMES; ++i) {
            const Cat *cat = mLM.setCategory(NAMES[i]);
            ASSERT(cat);
            ASSERT(0 == bsl::strcmp(NAMES[i], cat->categoryName()));
        }

        if (verbose) cout << endl << "\tTest New Category Threshold Levels"
                                  << " with No Inheritance" << endl;
        // Verify that when no threshold-level-values callback is installed,
        // all new categories take the default threshold values if they are
        // not specified.
        const char *HIERNAMES[] = {
            "A",
            "A.B",
            "A.B.C.D",
            "B.C",
            "B",
            "B.C.A",
            "B.A.C",
            "AA",
            "BB",
            "Aa.BB",
            "A.B.C.D.E.F.G"
        };
        const int NUM_HIERNAMES = sizeof HIERNAMES / sizeof *HIERNAMES;

        const int recLevel     = mLM.defaultRecordThresholdLevel();
        const int passLevel    = mLM.defaultPassThresholdLevel();
        const int trigLevel    = mLM.defaultTriggerThresholdLevel();
        const int trigAllLevel = mLM.defaultTriggerAllThresholdLevel();
        ASSERT(FACTORY_RECORD     == recLevel);
        ASSERT(FACTORY_PASS       == passLevel);
        ASSERT(FACTORY_TRIGGER    == trigLevel);
        ASSERT(FACTORY_TRIGGERALL == trigAllLevel);
        for (int i = 0; i < NUM_HIERNAMES; ++i) {
            const char *NAME = HIERNAMES[i];
            const Cat  *cat  = mLM.setCategory(NAME);
            ASSERT(cat);
            LOOP_ASSERT(i, 0            == bsl::strcmp(NAME,
                                                       cat->categoryName()));
            LOOP_ASSERT(i, recLevel     == cat->recordLevel());
            LOOP_ASSERT(i, passLevel    == cat->passLevel());
            LOOP_ASSERT(i, trigLevel    == cat->triggerLevel());
            LOOP_ASSERT(i, trigAllLevel == cat->triggerAllLevel());
        }

        // log a test message

        if (veryVerbose) testObserver.setVerbose(1);
        ball::Logger& lgr = mLM.getLogger();
        ASSERT(0 == testObserver.numPublishedRecords());
        const char *MESSAGE = "initSingleton variant 1";
        const int   LINE    = L_ + 1;
        lgr.logMessage(dfltCat, FACTORY_PASS, __FILE__, LINE, MESSAGE);
        ASSERT(1 == testObserver.numPublishedRecords());
        const Rec&  R = testObserver.lastPublishedRecord();
        if (veryVerbose) cout << R << endl;

        const Attr& A = R.fixedFields();
        ASSERT(0            == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                           A.category()));
        ASSERT(FACTORY_PASS == A.severity());
        ASSERT(0            == bsl::strcmp(__FILE__, A.fileName()));
        ASSERT(LINE         == A.lineNumber());
        ASSERT(0            == bsl::strcmp(MESSAGE, A.message()));

        const List& L = R.userFields();
        ASSERT(0 == L.length());

        ASSERT(1 == Obj::isInitialized());
        Obj::shutDownSingleton();
        ASSERT(0 == Obj::isInitialized());

      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   We want to demonstrate a base-line level of correct operation.
        //
        // Plan:
        // TBD doc
        //
        // Testing:
        //   This "test" *exercises* basic functionality, but *tests* nothing.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "BREATHING TEST" << endl
                                  << "==============" << endl;

        ball::TestObserver testObserver(cout);

        Cnf nameFilter(&toLower);

        Obj::DefaultThresholdLevelsCallback
                thresholdsCallback(&inheritThresholdLevels);

        bdlmxxx::Schema schema;
        bdlmxxx::RecordDef *p = schema.createRecord("FACTORIAL");
        p->appendField(bdlmxxx::ElemType::BDEM_INT, "n!");

        ball::Logger::UserPopulatorCallback populator(&myPopulator);

        ball::LoggerManagerConfiguration mLMC;
        mLMC.setCategoryNameFilterCallback(nameFilter);
        mLMC.setDefaultThresholdLevelsCallback(thresholdsCallback);
        mLMC.setUserFields(schema, populator);

        ball::LoggerManagerScopedGuard lmg(&testObserver, mLMC);
        ball::LoggerManager& mLM = Obj::singleton();

        ASSERT(&testObserver == mLM.observer());
        ASSERT(1             == mLM.numCategories());

        const Cat& dfltCat = mLM.defaultCategory();
        ASSERT(&dfltCat == mLM.lookupCategory(DEFAULT_CATEGORY_NAME));
        ASSERT(0 == bsl::strcmp(DEFAULT_CATEGORY_NAME,
                                dfltCat.categoryName()));
        ASSERT(FACTORY_RECORD     == dfltCat.recordLevel());
        ASSERT(FACTORY_PASS       == dfltCat.passLevel());
        ASSERT(FACTORY_TRIGGER    == dfltCat.triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == dfltCat.triggerAllLevel());

        const Cat *cat1 = mLM.addCategory("BLOOMBERG.BAE", 64, 48, 32, 16);
        ASSERT(cat1);
        ASSERT(2    == mLM.numCategories());
        ASSERT(cat1 == mLM.lookupCategory("BLOOMBERG.bae"));
        ASSERT( 0   == bsl::strcmp("bloomberg.bae", cat1->categoryName()));
        ASSERT(64   == cat1->recordLevel());
        ASSERT(48   == cat1->passLevel());
        ASSERT(32   == cat1->triggerLevel());
        ASSERT(16   == cat1->triggerAllLevel());

        const Cat *cat2 = mLM.setCategory("Bloomberg.BAE.LOGGING");
        ASSERT(cat2);
        ASSERT(3    == mLM.numCategories());
        ASSERT(cat2 == mLM.lookupCategory("BLOOMBERG.BAE.LOGGING"));
        ASSERT(0    == bsl::strcmp("bloomberg.bae.logging",
                                   cat2->categoryName()));
        ASSERT(64   == cat2->recordLevel());
        ASSERT(48   == cat2->passLevel());
        ASSERT(32   == cat2->triggerLevel());
        ASSERT(16   == cat2->triggerAllLevel());

        const Cat *cat3 = mLM.setCategory("BLOOMBERG");
        ASSERT(cat3);
        ASSERT(4                  == mLM.numCategories());
        ASSERT(cat3               == mLM.lookupCategory("Bloomberg"));
        ASSERT(0                  == bsl::strcmp("bloomberg",
                                                 cat3->categoryName()));
        ASSERT(FACTORY_RECORD     == cat3->recordLevel());
        ASSERT(FACTORY_PASS       == cat3->passLevel());
        ASSERT(FACTORY_TRIGGER    == cat3->triggerLevel());
        ASSERT(FACTORY_TRIGGERALL == cat3->triggerAllLevel());

        Cat *cat4 = mLM.setCategory(
               "Bloomberg.Bae.Logging.Component.LoggerManager.Test.Breathing",
               65, 49, 33, 17);
        ASSERT(cat4);
        ASSERT(5    == mLM.numCategories());
        ASSERT(cat4 == mLM.lookupCategory(
              "BLOOMBERG.bae.LOGGING.component.LOGGERMANAGER.test.BREATHING"));
        ASSERT(0    == bsl::strcmp(
                "bloomberg.bae.logging.component.loggermanager.test.breathing",
                cat4->categoryName()));
        ASSERT(65   == cat4->recordLevel());
        ASSERT(49   == cat4->passLevel());
        ASSERT(33   == cat4->triggerLevel());
        ASSERT(17   == cat4->triggerAllLevel());

        if (veryVerbose) {
            const char *categoryName = cat1->categoryName();
            int         record       = cat1->recordLevel();
            int         pass         = cat1->passLevel();
            int         trigger      = cat1->triggerLevel();
            int         triggerAll   = cat1->triggerAllLevel();
            P(categoryName);
            T_(); P_(record); P_(pass); P_(trigger); P(triggerAll);

            categoryName = cat2->categoryName();
            record       = cat2->recordLevel();
            pass         = cat2->passLevel();
            trigger      = cat2->triggerLevel();
            triggerAll   = cat2->triggerAllLevel();
            P(categoryName);
            T_(); P_(record); P_(pass); P_(trigger); P(triggerAll);

            categoryName = cat3->categoryName();
            record       = cat3->recordLevel();
            pass         = cat3->passLevel();
            trigger      = cat3->triggerLevel();
            triggerAll   = cat3->triggerAllLevel();
            P(categoryName);
            T_(); P_(record); P_(pass); P_(trigger); P(triggerAll);

            categoryName = cat4->categoryName();
            record       = cat4->recordLevel();
            pass         = cat4->passLevel();
            trigger      = cat4->triggerLevel();
            triggerAll   = cat4->triggerAllLevel();
            P(categoryName);
            T_(); P_(record); P_(pass); P_(trigger); P(triggerAll);
        }

        ball::Logger& lgr = mLM.getLogger();

        if (veryVerbose) testObserver.setVerbose(1);
        lgr.logMessage(*cat1, 64, __FILE__, __LINE__,
                       "First attempt to log to cat1.");

        lgr.logMessage(*cat2, my_Severity::BAEL_WARN, __FILE__, __LINE__,
                       "First attempt to log to cat2.");

        lgr.logMessage(*cat3, my_Severity::BAEL_WARN, __FILE__, __LINE__,
                       "First attempt to log to cat3.");

        cat4->setLevels(my_Severity::BAEL_TRACE,
                        my_Severity::BAEL_WARN,
                        my_Severity::BAEL_ERROR,
                        my_Severity::BAEL_FATAL);
        ASSERT(my_Severity::BAEL_TRACE == cat4->recordLevel());
        ASSERT(my_Severity::BAEL_WARN  == cat4->passLevel());
        ASSERT(my_Severity::BAEL_ERROR == cat4->triggerLevel());
        ASSERT(my_Severity::BAEL_FATAL == cat4->triggerAllLevel());

        lgr.logMessage(*cat4, my_Severity::BAEL_WARN, __FILE__, __LINE__,
                       "First attempt to log to cat4.");

        lgr.logMessage(*cat1, my_Severity::BAEL_WARN, __FILE__, __LINE__,
                       "Second attempt to log to cat1.");

        lgr.logMessage(*cat4, my_Severity::BAEL_ERROR, __FILE__, __LINE__,
                       "Second attempt to log to cat4.");

      } break;
      case -1: {
        // --------------------------------------------------------------------
        // TESTING: Warning about destroyed observer.
        //
        // Concerns:
        //  1 Verify that we log a message to 'stderr' if the registered
        //    observer is destroyed before the logger manager.
        //
        // Plan:
        //  1 Create an observer, and supply it to a logger manager, then
        //    destroy the observer prior to destroying the logger manager.
        //    Visually verify that output is written to stderr.
        //
        //  Note that this is a test of undefined behavior and must be run,
        //  and verified, manually.
        // --------------------------------------------------------------------

        if (verbose) cout << endl
                          << "Warning about destroyed observer." << endl
                          << "================================" << endl;

        {
            ball::TestObserver testObserver(cout);
            ball::LoggerManagerConfiguration lmc;

            ball::LoggerManager::initSingleton(&testObserver, lmc);
        }
        ball::LoggerManager::shutDownSingleton();

      } break;
      default: {
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
    }
    return testStatus;
}

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2004
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
