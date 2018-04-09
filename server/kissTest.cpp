

#include "kissdb.h"
#include "stackdb.h"
#include "dbCommon.h"
#include "minorGems/system/Time.h"

#include <malloc.h>

#include "minorGems/util/random/CustomRandomSource.h"

#define DB KISSDB
#define DB_open KISSDB_open
#define DB_close KISSDB_close
#define DB_get KISSDB_get
#define DB_put KISSDB_put
#define DB_Iterator  KISSDB_Iterator
#define DB_Iterator_init  KISSDB_Iterator_init
#define DB_Iterator_next  KISSDB_Iterator_next



CustomRandomSource randSource( 0 );



// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        }    
    }


int lastMallocCheck = 0;

int getMallocDelta() {
    struct mallinfo m = mallinfo();
    
    int current = m.hblkhd + m.usmblks;

    int d = current - lastMallocCheck;
    
    lastMallocCheck = current;
    
    return d;
    }


int main() {

    getMallocDelta();
    
    double startTime = Time::getCurrentTime();
    
    DB db;

    int tableSize = 80000;
    
    int error = DB_open( &db, 
                             "test.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             tableSize,
                             8, // two ints,  x_center, y_center
                             4 // one int
                             );

    printf( "Opening DB (table size %d) used %d bytes, took %f sec\n", 
            tableSize,
            getMallocDelta(),
            Time::getCurrentTime() - startTime );

    if( error ) {
        printf( "Failed to open\n" );
        return 1;
        }


    startTime = Time::getCurrentTime();

    int num = 1000;
    
    unsigned char key[8];
    unsigned char value[4];
    
    int insertCount = 0;
    for( int x=0; x<num; x++ ) {
        for( int y=0; y<num; y++ ) {
            
            insertCount++;
            
            intToValue( x + y, value );
            intPairToKey( x, y, key );
            
            //printf( "Inserting %d,%d\n", x, y );
            
            DB_put( &db, key, value );
            }
        }
    printf( "Inserted %d\n", insertCount );

    printf( "Inserts used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );


    startTime = Time::getCurrentTime();

    int lookupCount = insertCount / 10;
    int numRuns = 20;
    int numLooks = 0;
    int numHits = 0;
    
    unsigned int checksum = 0;

    for( int r=0; r<numRuns; r++ ) {
        CustomRandomSource runSource( 0 );

        for( int i=0; i<lookupCount; i++ ) {
            int x = runSource.getRandomBoundedInt( 0, num-1 );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            intPairToKey( x, y, key );
            int result = DB_get( &db, key, value );
            numLooks ++;
            if( result == 0 ) {
                int v = valueToInt( value );
                checksum += v;
                numHits++;
                }
            }
        }
    

    printf( "Random lookup for %d batchs of %d (%d/%d hits)\n", 
            numRuns, lookupCount, numHits, numLooks );

    printf( "Random look used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );

    printf( "Checksum = %u\n", checksum );
    




    startTime = Time::getCurrentTime();
    numLooks = 0;
    numHits = 0;

    for( int r=0; r<numRuns; r++ ) {
        CustomRandomSource runSource( 0 );

        for( int i=0; i<lookupCount; i++ ) {
            // these don't exist
            int x = runSource.getRandomBoundedInt( num + 10, num + num );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            intPairToKey( x, y, key );
            DB_get( &db, key, value );
            int result = DB_get( &db, key, value );
            numLooks ++;
            if( result == 0 ) {
                numHits++;
                }
            }
        }
    

    printf( "Random lookup for non-existing %d batchs of %d (%d/%d hits)\n", 
            numRuns, lookupCount, numHits, numLooks );

    printf( "Random look/miss used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );

    

    
    DB_Iterator dbi;

    startTime = Time::getCurrentTime();

    DB_Iterator_init( &db, &dbi );
    
    
    int count = 0;
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        count++;
        }
    printf( "Iterated %d\n", count );

    printf( "Iterating used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );


    DB_close( &db );

    return 0;
    }
