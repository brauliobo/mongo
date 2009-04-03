// test arbitration

var baseName = "jstests_pair3test";

ismaster = function( n ) {
    var im = n.getDB( "admin" ).runCommand( { "ismaster" : 1 } );
    print( "ismaster: " + tojson( im ) );
    assert( im, "command ismaster failed" );
    return im.ismaster;
}

connect = function() {
    startMongoProgram( "mongobridge", "--port", alPort, "--dest", "localhost:" + aPort );
    startMongoProgram( "mongobridge", "--port", arPort, "--dest", "localhost:" + aPort );
    startMongoProgram( "mongobridge", "--port", lpPort, "--dest", "localhost:" + lPort );
    startMongoProgram( "mongobridge", "--port", rpPort, "--dest", "localhost:" + rPort );
}

doTest = function() {
    ports = allocatePorts( 7 );
    aPort = ports[ 0 ];
    alPort = ports[ 1 ];
    arPort = ports[ 2 ];
    lPort = ports[ 3 ];
    lpPort = ports[ 4 ];
    rPort = ports[ 5 ];
    rpPort = ports[ 6 ];
    
    connect();
    
    a = startMongod( "--port", aPort, "--dbpath", "/data/db/" + baseName + "-arbiter", "--nohttpinterface" );
    l = startMongod( "--port", lPort, "--dbpath", "/data/db/" + baseName + "-left", "--pairwith", "127.0.0.1:" + rpPort, "127.0.0.1:" + alPort, "--oplogSize", "1", "--nohttpinterface" );
    r = startMongod( "--port", rPort, "--dbpath", "/data/db/" + baseName + "-right", "--pairwith", "127.0.0.1:" + lpPort, "127.0.0.1:" + arPort, "--oplogSize", "1", "--nohttpinterface" );
    
    // normal startup
    
    assert.soon( function() {
                lm = ismaster( l );
                rm = ismaster( r );
                
                assert( lm == -1 || lm == 0, "lm value invalid" );
                assert( rm == -1 || rm == 0 || rm == 1, "rm value invalid" );
                
                return ( lm == 0 && rm == 1 );
                } );

    // disconnect l (slave)
    
    stopMongoProgram( alPort );
    stopMongoProgram( lpPort );
    stopMongoProgram( rpPort );
    
    assert.soon( function() {
                lm = ismaster( l );
                rm = ismaster( r );
                
                assert( lm == 0 || lm == -3, "lm value invalid" );
                assert( rm == 1, "rm value invalid" );
                
                return ( lm == -3 );
                } );
    
    // disconnect r ( master )
    
    stopMongoProgram( arPort );
    
    assert.soon( function() {
                rm = ismaster( r );
                assert( rm == 1 || rm == -3, "rm value invalid" );
                return ( rm == -3 );
                } );
    
    // reconnect
    
    connect();
    assert.soon( function() {
                lm = ismaster( l );
                rm = ismaster( r );
                
                assert( lm == -3 || lm == 0, "lm value invalid" );
                assert( rm == -3 || rm == 1, "rm value invalid" );
                
                return ( lm == 0 && rm == 1 );
                } );

    // disconnect r ( master )
    stopMongoProgram( arPort );
    stopMongoProgram( lpPort );
    stopMongoProgram( rpPort );
    
    assert.soon( function() {
                lm = ismaster( l );
                rm = ismaster( r );
                
                assert( lm == 0 || lm == 1, "lm value invalid" );
                assert( rm == 1 || rm == -3, "rm value invalid" );
                
                return ( rm == -3 && lm == 1 );
                } );
    
    // disconnect l ( new master )
    stopMongoProgram( alPort );
    
    assert.soon( function() {
                lm = ismaster( l );
                assert( lm == 1 || lm == -3, "lm value invalid" );
                return ( lm == -3 );
                } );

}

doTest();
