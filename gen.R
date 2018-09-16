for( oid in seq(0,99999,2) ){
    initial_bid_price = sample (10:100,1 )
    initial_ask_price = sample (600:2000,1 )
    final_bid_price = sample (100:500,1 )
    final_ask_price = sample (480:100,1 )
    bid_volume = sample(1:8,1)
    ask_volume = sample(1:8,1)
    cat(paste("A,",oid,",B,",bid_volume,",",initial_bid_price,"\n",sep=""))
    cat(paste("A,",oid+1,",S,",ask_volume,",",initial_ask_price,"\n",sep=""))
    cat(paste("M,",oid,",B,",bid_volume,",",final_bid_price,"\n",sep=""))
    cat(paste("M,",oid+1,",S,",ask_volume,",",final_ask_price,"\n",sep=""))
}

