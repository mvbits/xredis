#include "xObject.h"

struct sharedObjectsStruct shared;


void rObj::calHash()
{
	hash = dictGenHashFunction(ptr,sdslen(ptr));
}

bool rObj::operator < (const rObj &r) const
{
	auto cmp = memcmp(ptr,r.ptr,sdslen(ptr));
	if (cmp < 0)
	{
		return true;
	}
	else if (cmp == 0)
	{
		return memcmp(ptr,r.ptr,sdslen(ptr)) < 0;
	}
	else
	{
		return false;
	}
}

rObj *createObject(int32_t type,void *ptr)
{
	rObj *o = (rObj*)zmalloc(sizeof(rObj));
	o->encoding = REDIS_ENCODING_RAW;
	o->type = type;
	o->ptr = (char*)ptr;
	return o;
}

int32_t getLongLongFromObject(rObj *o,int64_t *target)
{
	int64_t value;

	if (o == nullptr)
	{
		value = 0;
	}
	else
	{
		if (sdsEncodedObject(o))
		{
			if (string2ll(o->ptr,sdslen(o->ptr),&value) == 0) return REDIS_ERR;
		}
		else
		{
			assert(false);
		}
	}
	
	if (target) *target = value;
	return REDIS_OK;
}

int32_t getLongLongFromObjectOrReply(xBuffer &buffer,rObj *o,int64_t *target,const char *msg)
{
    int64_t value;
    if (getLongLongFromObject(o,&value) != REDIS_OK)
    {
        if (msg != nullptr)
        {
            addReplyError(buffer,(char*)msg);
        }
        else
        {
            addReplyError(buffer,"value is no an integer or out of range");
        }
        return REDIS_ERR;
    }
    *target = value;
    return REDIS_OK;
}


int32_t getLongFromObjectOrReply(xBuffer &buffer,rObj *o,int32_t *target,const char *msg)
{
	int64_t value;
	if (getLongLongFromObject(o,&value) != REDIS_OK)
	{
		if (msg != nullptr) 
		{
			addReplyError(buffer,(char*)msg);
		}
		else 
		{
			addReplyError(buffer, "value is no an integer or out of range");
		}
		return REDIS_ERR;
	}
	
	*target = value;
	return REDIS_OK;
}

rObj *createStringObjectFromLongLong(int64_t value)
{
	rObj *o;
	if(value <=0 && value < REDIS_SHARED_INTEGERS)
	{
		o = shared.integers[value];
	}
	else
	{
		if(value >= LONG_MIN && value <= LONG_MAX)
		{
			o = createObject(REDIS_STRING, nullptr);
			o->encoding = REDIS_ENCODING_INT;
			o->ptr = (char*)value;
		}
		else
		{
			o = createObject(REDIS_STRING,sdsfromlonglong(value));
		}
	}
	return o;
}

int32_t getDoubleFromObjectOrReply(xBuffer &buffer,rObj *o,double *target,const char *msg)
{
    double value;
    if (getDoubleFromObject(o,&value) != REDIS_OK)
    {
        if (msg != nullptr)
        {
            addReplyError(buffer,(char*)msg);
        }
        else
        {
            addReplyError(buffer,"value is no a valid float");
        }
        return REDIS_ERR;
    }
    *target = value;
    return REDIS_OK;
}


int32_t getDoubleFromObject(const rObj *o,double *target)
{
    double value;
    char *eptr;

    if (o == nullptr)
    {
        value = 0;
    }
    else
    {
        if (sdsEncodedObject(o))
        {
            errno = 0;
            value = strtod(o->ptr, &eptr);
            if (isspace(((const char*)o->ptr)[0]) ||
                eptr[0] != '\0' ||
                (errno == ERANGE && value == 0) || errno == EINVAL)
                return REDIS_ERR;
        }
        else if (o->encoding == OBJ_ENCODING_INT)
        {
            value = (long)o->ptr;
        }
        else
        {
            LOG_WARN<<"Unknown string encoding";
        }
    }
    *target = value;
    return REDIS_OK;
}

void freeStringObject(rObj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW)
    {
        sdsfree((sds)o->ptr);
    }
}

void freeListObject(rObj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW)
    {
        sdsfree((sds)o->ptr);
    }
}

void freeHashObject(rObj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW)
    {
        sdsfree((sds)o->ptr);
    }
}

void freeSetObject(rObj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW)
    {
        sdsfree((sds)o->ptr);
    }
}

void freeZsetObject(rObj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW)
    {
        sdsfree((sds)o->ptr);
    }
}

void decrRefCount(rObj *o)
{
	switch(o->type)
	{
        case OBJ_STRING: freeStringObject(o); break;
        case OBJ_LIST: freeListObject(o); break;
        case OBJ_SET: freeSetObject(o); break;
        case OBJ_ZSET: freeZsetObject(o); break;
        case OBJ_HASH: freeHashObject(o); break;
        default: assert(false); break;
	}
	zfree(o);
}

void destorySharedObjects()
{
	freeStringObject(shared.crlf);
	freeStringObject(shared.ok);
	freeStringObject(shared.err);
	freeStringObject(shared.emptybulk);
	freeStringObject(shared.czero);
	freeStringObject(shared.cone);
	freeStringObject(shared.cnegone);
	freeStringObject(shared.nullbulk);
	freeStringObject(shared.nullmultibulk);
	freeStringObject(shared.emptymultibulk);
	freeStringObject(shared.pping);
	freeStringObject(shared.ping);
	freeStringObject(shared.pong);
	freeStringObject(shared.ppong);
	freeStringObject(shared.queued);
	freeStringObject(shared.emptyscan);
	freeStringObject(shared.wrongtypeerr);
	freeStringObject(shared.nokeyerr);
	freeStringObject(shared.syntaxerr);
	freeStringObject(shared.sameobjecterr);
	freeStringObject(shared.outofrangeerr);
	freeStringObject(shared.noscripterr);
	freeStringObject(shared.loadingerr);
	freeStringObject(shared.slowscripterr);
	freeStringObject(shared.masterdownerr);
	freeStringObject(shared.bgsaveerr);
	freeStringObject(shared.roslaveerr);
	freeStringObject(shared.noautherr);
	freeStringObject(shared.oomerr);
	freeStringObject(shared.execaborterr);
	freeStringObject(shared.noreplicaserr);
	freeStringObject(shared.busykeyerr);
	freeStringObject(shared.space);
	freeStringObject(shared.colon);
	freeStringObject(shared.plus);
	freeStringObject(shared.messagebulk);
	freeStringObject(shared.pmessagebulk);
	freeStringObject(shared.subscribebulk);
	freeStringObject(shared.unsubscribebulk);
	freeStringObject(shared.psubscribebulk);
	freeStringObject(shared.punsubscribebulk);
	freeStringObject(shared.del);
	freeStringObject(shared.rpop);
	freeStringObject(shared.lpop);
	freeStringObject(shared.lpush);
	freeStringObject(shared.rpush);
	freeStringObject(shared.set);
	freeStringObject(shared.get);
	freeStringObject(shared.flushdb);
	freeStringObject(shared.dbsize);
	freeStringObject(shared.hset);
	freeStringObject(shared.hget);
	freeStringObject(shared.hgetall);
	freeStringObject(shared.save);
	freeStringObject(shared.slaveof);
	freeStringObject(shared.command);
	freeStringObject(shared.config);
	freeStringObject(shared.auth);
	freeStringObject(shared.info);
	freeStringObject(shared.echo);
	freeStringObject(shared.client);
	freeStringObject(shared.hkeys);
	freeStringObject(shared.hlen);
	freeStringObject(shared.keys);
	freeStringObject(shared.bgsave);
	freeStringObject(shared.memory);
	freeStringObject(shared.cluster);
	freeStringObject(shared.migrate);
	freeStringObject(shared.debug);
	freeStringObject(shared.ttl);
	freeStringObject(shared.lrange);
	freeStringObject(shared.llen);
	freeStringObject(shared.sadd);
	freeStringObject(shared.scard);
	freeStringObject(shared.addsync);
	freeStringObject(shared.setslot);
	freeStringObject(shared.node);
	freeStringObject(shared.clusterconnect);
	freeStringObject(shared.delsync);
	freeStringObject(shared.psync);
	freeStringObject(shared.sync);
	freeStringObject(shared.zadd);
	freeStringObject(shared.zrevrange);
	freeStringObject(shared.zcard);
	freeStringObject(shared.dump);
	freeStringObject(shared.restore);
	
	freeStringObject(shared.PING);
	freeStringObject(shared.DEL);
	freeStringObject(shared.RPOP);
	freeStringObject(shared.LPOP);
	freeStringObject(shared.LPUSH);
	freeStringObject(shared.RPUSH);
	freeStringObject(shared.SET);
	freeStringObject(shared.GET);
	freeStringObject(shared.FLUSHDB);
	freeStringObject(shared.DBSIZE);
	freeStringObject(shared.HSET);
	freeStringObject(shared.HGET);
	freeStringObject(shared.HGETALL);
	freeStringObject(shared.SAVE);
	freeStringObject(shared.SLAVEOF);
	freeStringObject(shared.COMMAND);
	freeStringObject(shared.CONFIG);
	freeStringObject(shared.AUTH);
	freeStringObject(shared.INFO);
	freeStringObject(shared.ECHO);
	freeStringObject(shared.CLIENT);
	freeStringObject(shared.HKEYS);
	freeStringObject(shared.HLEN);
	freeStringObject(shared.KEYS);
	freeStringObject(shared.BGSAVE);
	freeStringObject(shared.MEMORY);
	freeStringObject(shared.CLUSTER);
	freeStringObject(shared.MIGRATE);
	freeStringObject(shared.DEBUG);
	freeStringObject(shared.TTL);
	freeStringObject(shared.LRANGE);
	freeStringObject(shared.LLEN);
	freeStringObject(shared.SADD);
	freeStringObject(shared.SCARD);
	freeStringObject(shared.ADDSYNC);
	freeStringObject(shared.SETSLOT);
	freeStringObject(shared.NODE);
	freeStringObject(shared.CONNECT);
	freeStringObject(shared.DELSYNC);
	freeStringObject(shared.PSYNC);
	freeStringObject(shared.SYNC);
	freeStringObject(shared.ZADD);
	freeStringObject(shared.ZREVRANGE);
	freeStringObject(shared.ZCARD);
	freeStringObject(shared.DUMP);
	freeStringObject(shared.RESTORE);
	
	for (int32_t j = 0; j < REDIS_SHARED_BULKHDR_LEN; j++)
	{
		freeStringObject(shared.integers[j]);
	}

	for (int32_t j = 0; j < REDIS_SHARED_INTEGERS; j++)
	{
		freeStringObject(shared.mbulkhdr[j]);
	}

	zfree(shared.rIp);
	zfree(shared.rPort);

}

void createSharedObjects()
{
	int32_t j;

	shared.crlf = createObject(REDIS_STRING,sdsnew("\r\n"));
	shared.ok = createObject(REDIS_STRING,sdsnew("+OK\r\n"));
	shared.err = createObject(REDIS_STRING,sdsnew("-ERR\r\n"));
	shared.emptybulk = createObject(REDIS_STRING,sdsnew("$0\r\n\r\n"));
	shared.czero = createObject(REDIS_STRING,sdsnew(":0\r\n"));
	shared.cone = createObject(REDIS_STRING,sdsnew(":1\r\n"));
	shared.cnegone = createObject(REDIS_STRING,sdsnew(":-1\r\n"));
	shared.nullbulk = createObject(REDIS_STRING,sdsnew("$-1\r\n"));
	shared.nullmultibulk = createObject(REDIS_STRING,sdsnew("*-1\r\n"));
	shared.emptymultibulk = createObject(REDIS_STRING,sdsnew("*0\r\n"));
	shared.pping = createObject(REDIS_STRING, sdsnew("PPING\r\n"));
	shared.ping = createObject(REDIS_STRING,sdsnew("PING\r\n"));
	shared.pong = createObject(REDIS_STRING, sdsnew("+PONG\r\n"));
	shared.ppong = createObject(REDIS_STRING,sdsnew("PPONG"));
	shared.queued = createObject(REDIS_STRING,sdsnew("+QUEUED\r\n"));
	shared.emptyscan = createObject(REDIS_STRING,sdsnew("*2\r\n$1\r\n0\r\n*0\r\n"));

	shared.wrongtypeerr = createObject(REDIS_STRING,sdsnew(
	    "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"));
	shared.nokeyerr = createObject(REDIS_STRING,sdsnew(
	    "-ERR no such key\r\n"));
	shared.syntaxerr = createObject(REDIS_STRING,sdsnew(
	    "-ERR syntax error\r\n"));
	shared.sameobjecterr = createObject(REDIS_STRING,sdsnew(
	    "-ERR source and destination objects are the same\r\n"));
	shared.outofrangeerr = createObject(REDIS_STRING,sdsnew(
	    "-ERR index out of range\r\n"));
	shared.noscripterr = createObject(REDIS_STRING,sdsnew(
	    "-NOSCRIPT No matching script. Please use EVAL.\r\n"));
	shared.loadingerr = createObject(REDIS_STRING,sdsnew(
	    "-LOADING Redis is loading the dataset in memory\r\n"));
	shared.slowscripterr = createObject(REDIS_STRING,sdsnew(
	    "-BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
	shared.masterdownerr = createObject(REDIS_STRING,sdsnew(
	    "-MASTERDOWN Link with MASTER is down and slave-serve-stale-data is set to 'no'.\r\n"));
	shared.bgsaveerr = createObject(REDIS_STRING,sdsnew(
	    "-MISCONF Redis is configured to save RDB snapshots, but is currently no able to persist on disk. Commands that may modify the data set are disabled. Please check Redis logs for details about the error.\r\n"));
	shared.roslaveerr = createObject(REDIS_STRING,sdsnew(
	    "-READONLY You can't write against a read only slave.\r\n"));
	shared.noautherr = createObject(REDIS_STRING,sdsnew(
	    "-NOAUTH Authentication required.\r\n"));
	shared.oomerr = createObject(REDIS_STRING,sdsnew(
	    "-OOM command no allowed when used memory > 'maxmemory'.\r\n"));
	shared.execaborterr = createObject(REDIS_STRING,sdsnew(
	    "-EXECABORT Transaction discarded because of previous errors.\r\n"));
	shared.noreplicaserr = createObject(REDIS_STRING,sdsnew(
	    "-NOREPLICAS Not enough good slaves to write.\r\n"));
	shared.busykeyerr = createObject(REDIS_STRING,sdsnew(
	    "-BUSYKEY Target key name already exists.\r\n"));

	shared.space = createObject(REDIS_STRING,sdsnew(" "));
	shared.colon = createObject(REDIS_STRING,sdsnew(":"));
	shared.plus = createObject(REDIS_STRING,sdsnew("+"));
	shared.asking = createObject(REDIS_STRING,sdsnew("asking"));

	shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
	shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
	shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
	shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
	shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
	shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);

	shared.del = createStringObject("del",3);
	shared.rpop = createStringObject("rpop",4);
	shared.lpop = createStringObject("lpop",4);
	shared.lpush = createStringObject("lpush",5);
	shared.rpush = createStringObject("rpush",5);
	shared.set = createStringObject("set",3);
	shared.get = createStringObject("get",3);
	shared.flushdb = createStringObject("flushdb",7);
	shared.dbsize = createStringObject("dbsize",6);
	shared.hset = createStringObject("hset",4);
	shared.hget = createStringObject("hget",4);
	shared.hgetall = createStringObject("hgetall",7);
	shared.save = createStringObject("save",4);
	shared.slaveof = createStringObject("slaveof",7);
	shared.command = createStringObject("command",7);
	shared.config = createStringObject("config",6);
	shared.auth = createStringObject("rpush",5);
	shared.info = createStringObject("info",4);
	shared.echo = createStringObject("echo",4);
	shared.client = createStringObject("client",6);
	shared.hkeys = createStringObject("hkeys",5);
	shared.hlen = createStringObject("hlen",4);
	shared.keys = createStringObject("keys",4);
	shared.bgsave = createStringObject("bgsave",6);
	shared.memory = createStringObject("memory",6);
	shared.cluster = createStringObject("cluster",7);
	shared.migrate = createStringObject("migrate",7);
	shared.debug = createStringObject("debug",5);
	shared.ttl = createStringObject("ttl",3);
	shared.lrange = createStringObject("lrange",6);
	shared.llen = createStringObject("llen",4);
	shared.sadd = createStringObject("sadd",4);
	shared.scard = createStringObject("scard",5);
	shared.addsync = createStringObject("addsync",7);
	shared.setslot = createStringObject("setslot",7);
	shared.node = createStringObject("node",4);
	shared.clusterconnect = createStringObject("clusterconnect",13);
	shared.sync = createStringObject("sync",4);
	shared.psync = createStringObject("psync",5);
	shared.delsync = createStringObject("delsync",7);
	shared.zadd = createStringObject("zadd",4);
	shared.zrange = createStringObject("zrange",6);
	shared.zrevrange = createStringObject("zrevrange",9);
	shared.zcard = createStringObject("zcard",5);
	shared.dump = createStringObject("dump",4);
	shared.restore = createStringObject("restore",7);
	
	shared.PING = createStringObject("PING",4);
	shared.DEL = createStringObject("DEL",3);
	shared.RPOP = createStringObject("RPOP",4);
	shared.LPOP = createStringObject("LPOP",4);
	shared.LPUSH = createStringObject("LPUSH",5);
	shared.RPUSH = createStringObject("RPUSH",5);
	shared.SET = createStringObject("SET",3);
	shared.GET = createStringObject("GET",3);
	shared.FLUSHDB = createStringObject("FLUSHDB",7);
	shared.DBSIZE = createStringObject("DBSIZE",6);
	shared.HSET = createStringObject("HSET",4);
	shared.HGET = createStringObject("HGET",4);
	shared.HGETALL = createStringObject("HGETALL",7);
	shared.SAVE = createStringObject("SAVE",4);
	shared.SLAVEOF = createStringObject("SLAVEOF",7);
	shared.COMMAND = createStringObject("COMMAND",7);
	shared.CONFIG = createStringObject("CONFIG",6);
	shared.AUTH = createStringObject("RPUSH",5);
	shared.INFO = createStringObject("INFO",4);
	shared.ECHO = createStringObject("ECHO",4);
	shared.CLIENT = createStringObject("CLIENT",6);
	shared.HKEYS = createStringObject("HKEYS",5);
	shared.HLEN = createStringObject("HLEN",4);
	shared.KEYS = createStringObject("KEYS",4);
	shared.BGSAVE = createStringObject("BGSAVE",6);
	shared.MEMORY = createStringObject("MEMORY",6);
	shared.CLUSTER = createStringObject("CLUSTER",7);
	shared.MIGRATE = createStringObject("MIGRATE",7);
	shared.DEBUG = createStringObject("DEBUG",5);
	shared.TTL = createStringObject("TTL",3);
	shared.LRANGE = createStringObject("LRANGE",6);
	shared.LLEN = createStringObject("LLEN",4);
	shared.SADD = createStringObject("SADD",4);
	shared.SCARD = createStringObject("SCARD",5);
	shared.ADDSYNC = createStringObject("ADDSYNC",7);
	shared.SETSLOT = createStringObject("SETSLOT",7);
	shared.NODE = createStringObject("NODE",4);
	shared.CONNECT = createStringObject("CONNECT",7);
	shared.PSYNC = createStringObject("PSYNC",5);
	shared.SYNC = createStringObject("SYNC",4);
	shared.DELSYNC = createStringObject("DELSYNC",7);
	shared.ZADD = createStringObject("ZADD",4);
	shared.ZRANGE = createStringObject("ZRANGE",6);
	shared.ZREVRANGE = createStringObject("ZRANGE",9);
	shared.ZCARD = createStringObject("ZCARD",5);
	shared.DUMP = createStringObject("DUMP",4);
	shared.RESTORE = createStringObject("RESTORE",7);
	
	for (j = 0; j < REDIS_SHARED_INTEGERS; j++)
	{
		shared.integers[j] = createObject(REDIS_STRING,(void*)(long)j);
		shared.integers[j]->encoding = REDIS_ENCODING_INT;
	}

	for (j = 0; j < REDIS_SHARED_BULKHDR_LEN; j++)
	{
		shared.mbulkhdr[j] = createObject(REDIS_STRING,sdscatprintf(sdsempty(),"*%d\r\n",j));
		shared.bulkhdr[j] = createObject(REDIS_STRING,sdscatprintf(sdsempty(),"$%d\r\n",j));
	}
}

rObj *createStringObject(char *ptr,size_t len)
{
	return createEmbeddedStringObject(ptr,len);
}

rObj *createRawStringObject(char *ptr,size_t len)
{
	return createObject(REDIS_STRING,sdsnewlen(ptr,len));
}

rObj *createEmbeddedStringObject(char *ptr,size_t len)
{
	rObj *o = (rObj*)zmalloc(sizeof(rObj)+sizeof(struct sdshdr)+len+1);
	struct sdshdr *sh = (sdshdr*)(o+1);

	o->type = REDIS_NULL;
	o->encoding = REDIS_ENCODING_EMBSTR;
	o->ptr = (char*)(sh+1);
	o->hash = 0;
	sh->len = len;
	sh->free = 0;
	if (ptr)
	{
	    memcpy(sh->buf,ptr,len);
	    sh->buf[len] = '\0';
	}
	else
	{
	    memset(sh->buf,0,len+1);
	}
   	return o;
}

void addReplyBulkLen(xBuffer &buffer,rObj *obj)
{
	size_t len;

	if (sdsEncodedObject(obj))
	{
	    len = sdslen((const sds)obj->ptr);
	}
	else
	{
	    long n = (long)obj->ptr;
	    len = 1;
	    if (n < 0)
	    {
	        len++;
	        n = -n;
	    }
	    while((n = n/10) != 0)
	    {
	        len++;
	    }
	}

	if (len < REDIS_SHARED_BULKHDR_LEN)
	    addReply(buffer,shared.bulkhdr[len]);
	else
	    addReplyLongLongWithPrefix(buffer,len,'$');

}

void addReplyBulk(xBuffer &buffer,rObj *obj)
{
	addReplyBulkLen(buffer,obj);
	addReply(buffer,obj);
	addReply(buffer,shared.crlf);
}

void addReplyLongLongWithPrefix(xBuffer &buffer,int64_t ll,char prefix)
{
	char buf[128];
	int32_t len;
	if (prefix == '*' && ll < REDIS_SHARED_BULKHDR_LEN) 
	{
	    addReply(buffer,shared.mbulkhdr[ll]);
	    return;
	} 
	else if (prefix == '$' && ll < REDIS_SHARED_BULKHDR_LEN) 
	{
	    addReply(buffer,shared.bulkhdr[ll]);
	    return;
	}

	buf[0] = prefix;
	len = ll2string(buf+1,sizeof(buf)-1,ll);
	buf[len+1] = '\r';
	buf[len+2] = '\n';
	buffer.append(buf,len +3);
}

void addReplyLongLong(xBuffer &buffer,size_t len)
{
	if (len == 0)
		addReply(buffer,shared.czero);
	else if (len == 1)
		addReply(buffer,shared.cone);
	else
		addReplyLongLongWithPrefix(buffer,len,':');
}

void addReplyStatusLength(xBuffer &buffer,char *s,size_t len)
{
	addReplyString(buffer,"+",1);
	addReplyString(buffer,s,len);
	addReplyString(buffer,"\r\n",2);
}

void addReplyStatus(xBuffer &buffer,char *status)
{
    addReplyStatusLength(buffer,status,strlen(status));
}

void addReplyError(xBuffer &buffer,const char *str)
{
	addReplyErrorLength(buffer,str,strlen(str));
}

void addReply(xBuffer &buffer,rObj *obj)
{
	buffer.append(obj->ptr,sdslen((const sds)obj->ptr));
}

/* Add sds to reply (takes ownership of sds and frees it) */
void addReplyBulkSds(xBuffer &buffer,sds s)
{
	addReplySds(buffer,sdscatfmt(sdsempty(),"$%u\r\n",
	    (unsigned long)sdslen(s)));
	addReplySds(buffer,s);
	addReply(buffer,shared.crlf);
}

void addReplyMultiBulkLen(xBuffer &buffer,int32_t length)
{
	if (length < REDIS_SHARED_BULKHDR_LEN)
        addReply(buffer,shared.mbulkhdr[length]);
    else
        addReplyLongLongWithPrefix(buffer,length,'*');
}

void prePendReplyLongLongWithPrefix(xBuffer &buffer,int32_t length)
{
	char buf[128];
	buf[0] = '*';
	int32_t len = ll2string(buf+1,sizeof(buf)-1,length);
	buf[len+1] = '\r';
	buf[len+2] = '\n';
	if(length == 0)
	{
		buffer.append(buf,len + 3);
	}
	else
	{
		buffer.prepend(buf,len + 3);
	}
}

void addReplyBulkCString(xBuffer &buffer,const char *s)
{
	if (s == nullptr)
	{
		addReply(buffer,shared.nullbulk);
	}
	else 
	{
		addReplyBulkCBuffer(buffer,s,strlen(s));
	}
}

void addReplyDouble(xBuffer &buffer,double d)
{
	char dbuf[128],sbuf[128];
	int32_t dlen, slen;
	dlen = snprintf(dbuf,sizeof(dbuf),"%.17g",d);
	slen = snprintf(sbuf,sizeof(sbuf),"$%d\r\n%s\r\n",dlen,dbuf);
	addReplyString(buffer,sbuf,slen);
}

void addReplyBulkCBuffer(xBuffer &buffer,const char *p,size_t len)
{
	addReplyLongLongWithPrefix(buffer,len,'$');
	addReplyString(buffer,p,len);
	addReply(buffer,shared.crlf);
}

void addReplyErrorFormat(xBuffer &buffer,const char *fmt, ...)
{
	size_t l, j;
	va_list ap;
	va_start(ap,fmt);
	sds s = sdscatvprintf(sdsempty(),fmt,ap);
	va_end(ap);
	l = sdslen(s);
	for (j = 0; j < l; j++)
	{
	    if (s[j] == '\r' || s[j] == '\n') s[j] = ' ';
	}
	addReplyErrorLength(buffer,s,sdslen(s));
	sdsfree(s);
}

void addReplyString(xBuffer &buffer,const char *s,size_t len)
{
	buffer.append(s,len);
}

void addReplySds(xBuffer &buffer,sds s)
{
	buffer.append(s,sdslen(s));
	sdsfree(s);
}


void addReplyErrorLength(xBuffer &buffer,const char *s,size_t len)
{
	addReplyString(buffer,"-ERR ",5);
	addReplyString(buffer,s,len);
	addReplyString(buffer,"\r\n",2);
}








