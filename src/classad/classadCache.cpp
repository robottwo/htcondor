/***************************************************************
 * 
 * Copyright 2012 Red Hat, Inc. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you 
 * may not use this file except in compliance with the License.  You may 
 * obtain a copy of the License at 
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and 
 * limitations under the License. 
 * 
 ***************************************************************/

#include "classad/classadCache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <list>

#ifdef WIN32
#include <process.h> 
#define _getpid getpid
#endif

using namespace classad;
using namespace std;

/**
 * ClassAdCache - is meant to be the storage container which is used to cache classads,
 * I've tried some fancy tricks but they don't actually yield much better performance 
 * characteristics so I've decided to keep it simple stupid (KISS)
 * 
 * @author Timothy St. Clair
 */
class ClassAdCache
{
protected:
    
	typedef classad_unordered<CacheKey*, pCacheEntry, CacheKeyHash, CacheKeyEq> AttrCache;
	typedef AttrCache::iterator cache_iterator;

	AttrCache m_Cache;		///< Data Store
	unsigned long m_HitCount;	///< Hit Counter
	unsigned long m_MissCount;	///< Miss Counter
	unsigned long m_MissCheck;	///< Miss Counter
	unsigned long m_RemovalCount;	///< Useful to see churn
	
public:
	ClassAdCache()
	{ 
	  m_HitCount = 0; 
	  m_MissCount = 0;
	  m_MissCheck = 0;
	  m_RemovalCount = 0;
	};
	
	virtual ~ClassAdCache(){;};

	///< cache's a local attribute->ExpTree
#ifndef WIN32
	pCacheData cache( std::string & szName, const std::string & szValue , ExprTree * pVal)
#else
	pCacheData cache(const std::string & szName, const std::string & szValue , ExprTree * pVal)
#endif
	{
		pCacheData pRet;
		CompactExpr packer;
                bool canPack = pVal; if (canPack) packer.Pack(*pVal);
		std::string qString = (canPack ? packer.getData() : szValue);
		//printf("Leaking packed string of size %lu\n", qString.size());
		//std::string *leak_str = new std::string(qString);
		//delete pVal;
		//return NULL;
		//if (!pVal) return NULL; else {delete pVal; return NULL;}
		//else {delete pVal; return NULL;}
		CacheEntry *entry = new CacheEntry(szName, qString, canPack ? NULL : pVal );
		CacheKey *key = entry->getKey();
		cache_iterator itr = m_Cache.find(key);
		
		
		if ( itr != m_Cache.end() ) 
		{
                    pRet = itr->second.lock();
                    
                    m_HitCount++;
		    std::string key_val; key_val.reserve(key->getLength()); key_val.replace(0, key->getOffset(), key->getData(), key->getOffset());
		    printf("Hit count: %lu, value size %u, key: %s\n", m_HitCount, key->getLength() - key->getOffset(), key_val.c_str());
                    delete entry;
                    if (pVal)
                    {
                        delete pVal;
                    }
                    
                    // don't do any more checks just return.
                    return pRet;
		} 
		
		// if we got here we missed 
                if (pVal)
                {
                    if (canPack) {delete pVal;}
                    pRet.reset(entry);
                    m_Cache[key] = pRet;

                    m_MissCount++;
		    printf("Miss count: %lu\n", m_MissCount);
                }
                else
                {
                    m_MissCheck++;
                }

		return pRet;
	}

	///< clears a cache key
	bool flush(const std::string & szName, const std::string & szValue)
	{
/*
	  cache_iterator itr = m_Cache.find(szName);

      if (itr != m_Cache.end())
	  {
		  if (itr->second.size() == 1)
			  {
				m_Cache.erase(itr);
			  }
			  else
			  {
				value_iterator vtr = itr->second.find(szValue);
				itr->second.erase(vtr);
		      }

		  m_RemovalCount++;
		  return (true);
	  }
*/
	  return false;
	} 

	bool flush(CacheKey *key)
	{
		cache_iterator itr = m_Cache.find(key);
		if (itr != m_Cache.end())
		{
			m_Cache.erase(itr);
			m_RemovalCount++;
			return true;
		}
		return false;
	}
	
	///< dumps the contents of the cache to the file
	bool dump_keys(const std::string & szFile)
	{
	  FILE * fp = fopen ( szFile.c_str(), "a+" );
	  bool bRet = false;
/*
	  if (fp)
	  {
	    double dHitRatio = (m_HitCount/ ((double)m_HitCount+m_MissCount))*100;
	    double dMissRatio = (m_MissCount/ ((double)m_HitCount+m_MissCount))*100;
	    unsigned long lTotalUseCount=0;
	    unsigned long lTotalPruned=0;
	    unsigned long lEntries=0;

	    cache_iterator itr = m_Cache.begin();

	    while ( itr != m_Cache.end() )
	    {
              value_iterator vtr = itr->second.begin();
              
              while (vtr != itr->second.end())
              {
                if (vtr->second.expired())
                {
                    // this should never happen.
                    fprintf( fp, "EXPIRED ** %s = %s\n", itr->first.c_str(), vtr->first.c_str() );
                    vtr = itr->second.erase(vtr);
                    lTotalPruned++;
                }
                else
                {
                    lTotalUseCount += vtr->second.use_count();
                    lEntries++;
                    
                    // it's written directly to a file b/c it has the potential to be very large 
                    fprintf( fp, "[%s = %s] - %lu\n", itr->first.c_str(), vtr->first.c_str(), vtr->second.use_count() );
                    vtr++;
                }
              }
              
              itr++;
	    }

	    // written at the end so you can tail the file.
	    fprintf( fp, "------------------------------------------------\n");
	    fprintf( fp, "ClassAdCache data for PID(%d)\n", getpid() ); 
	    fprintf( fp, "Hits [%lu - %f] Misses[%lu - %f] QueryMiss[%lu]\n", m_HitCount,dHitRatio,m_MissCount,dMissRatio,m_MissCheck ); 
	    fprintf( fp, "Entries[%lu] UseCount[%lu] FlushedCount[%lu]\n", lEntries,lTotalUseCount,m_RemovalCount );
	    fprintf( fp, "Pruned[%lu] - SHOULD BE 0\n",lTotalPruned);
	    fprintf( fp, "------------------------------------------------\n");
	    fclose(fp);

	    bRet = true;

	  }
*/
	  return (bRet);
	}

	///< dumps the contents of the cache to the file
	void print_stats(FILE* fp)
	{
		double dHitRatio = 0.0;
		double dMissRatio = 0.0;
		unsigned long cTotalUseCount = 0;
		unsigned long cMaxUseCount = 0;
		unsigned long cTotalPruned = 0;
		unsigned long cTotalValues = 0;
		unsigned long cAttribs = 0;
		unsigned long cSingletonValues = 0;
		unsigned long cAttribsWithOnlySingletonValues = 0;
		unsigned long cSingletonAttribs = 0;

		if (m_HitCount+m_MissCount) {
			double dTot = m_HitCount + m_MissCount;
			dHitRatio = (100.0 * m_HitCount) / dTot;
			dMissRatio = (100.0 * m_MissCount) / dTot;
		}
/*
		cache_iterator itr = m_Cache.begin();
		while (itr != m_Cache.end())
		{
			value_iterator vtr = itr->second.begin();

			unsigned long cValues = 0;
			unsigned long cMaxUse = 0;
			while (vtr != itr->second.end())
			{
				unsigned long cUseCount = vtr->second.use_count();
				if (cUseCount == 1) { ++cSingletonValues; }
				cTotalUseCount += cUseCount;
				if (cMaxUse < cUseCount) { cMaxUse = cUseCount; }

				++cTotalValues;
				++cValues;
				vtr++;
			}

			if (cMaxUseCount < cMaxUse) { cMaxUseCount = cMaxUse; }
			if (cMaxUse <= 1) { ++cAttribsWithOnlySingletonValues; }
			if (cValues <= 1) { ++cSingletonAttribs; }

			++cAttribs;
			itr++;
		}
*/
		fprintf( fp, "Attribs: %lu SingleUseAttribs: %lu AttribsWithOnlySingletons: %lu\n",  cAttribs, cSingletonAttribs, cAttribsWithOnlySingletonValues);
		fprintf( fp, "Values: %lu SingleUseValues: %lu UseCountTot:%lu UseCountMax: %lu\n", cTotalValues, cSingletonValues, cTotalUseCount, cMaxUseCount);
		fprintf( fp, "Hits:%lu (%.2f%%) Misses: %lu (%.2f%%) QueryMiss: %lu\n", m_HitCount,dHitRatio,m_MissCount,dMissRatio,m_MissCheck ); 
	};

};

static classad_shared_ptr<ClassAdCache> _cache;

CacheKey::CacheKey(const std::string &name, const std::string &val)
{
    m_offset = name.size();
    m_length = m_offset + val.size();
    m_data = new char[m_length];
    memcpy(m_data, name.c_str(), m_offset);
    memcpy(m_data+m_offset, val.c_str(), val.size());
}

CacheKey::~CacheKey()
{
    _cache->flush(this);
    delete m_data;
}

size_t
CacheKeyHash::operator()( const CacheKey *s ) const
{
    size_t h = 0;
    unsigned char const *ch = (unsigned const char*)s->m_data;
    for( unsigned idx=0; idx<s->m_length; idx++ ) {
        h = 5*h + (*ch | 0x20);
        ch++;
    }
    return h;
}

bool
CacheKeyEq::operator()(const CacheKey *s1, const CacheKey *s2) const
{
    if (s1->m_offset != s2->m_offset) return false;
    if (s1->m_length != s2->m_length) return false;
    if (strncasecmp(s1->m_data, s2->m_data, s1->m_offset) != 0) return false;
    if (memcmp(s1->m_data+s1->m_offset, s2->m_data+s2->m_offset, s2->m_length-s2->m_offset) != 0) return false;
    return true;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CacheEntry::CacheEntry()
    : m_key("", "")//,
      //pData(NULL)
{
}

CacheEntry::CacheEntry(const std::string &szName, const std::string &szValue, ExprTree * pDataIn)
    : m_key(szName, szValue)//,
      //pData(pDataIn)
{
    //if (pDataIn) pData = pDataIn;
    //else pData = NULL;
}

CacheEntry::~CacheEntry()
{
/*
    if (bExpr && pData)
    {
        _cache->flush(szName, szValue);
        delete pData;
        pData=0;
    }
    else if (!bExpr)
    {
        _cache->flush(szName, szValue);
        cExpr.~CompactExpr();
    }
*/
}

CachedExprEnvelope::~CachedExprEnvelope()
{
  // nothing to do shifted to the cache entry. 
}

#ifndef WIN32
ExprTree * CachedExprEnvelope::cache (std::string & pName, const std::string & szValue, ExprTree * pTree)
#else
ExprTree * CachedExprEnvelope::cache (const std::string & pName, const std::string & szValue, ExprTree * pTree)
#endif
{
	ExprTree * pRet=pTree;
	const std::string * pszValue = &szValue;
	NodeKind nk = pTree->GetKind();
	
	switch (nk)
	{
	  case EXPR_ENVELOPE:
	     pRet = pTree;
	  break;
	  
	  case EXPR_LIST_NODE:
	  case CLASSAD_NODE:
#ifndef WIN32 // this just wastes time on windows.
	    // for classads the values are already cached but we still should string space the name
	    check_hit (pName, szValue);
#endif
	  break;
	    
	  default:
	  {
	    CachedExprEnvelope * pNewEnv = new CachedExprEnvelope();
	
		// if no unparsed value was passed in, then unparse the passed-in ExprTree
		std::string szUnparsedValue;
		if (szValue.empty()) {
			classad::val_str(szUnparsedValue, pTree);
			pszValue = &szUnparsedValue;
		}
	
	    pNewEnv->nodeKind = EXPR_ENVELOPE;
	    if (!_cache)
	    {
	      _cache.reset( new ClassAdCache() );
	    }

	    pNewEnv->m_pLetter = _cache->cache(pName, *pszValue, pTree);
	    pRet = pNewEnv;
            //delete pNewEnv;
            //pRet = NULL;
	  }
	}
	
	return ( pRet );
}

bool CachedExprEnvelope::_debug_dump_keys(const string & szFile)
{
  if ( ! _cache) return false;
  return _cache->dump_keys(szFile);
}

void CachedExprEnvelope::_debug_print_stats(FILE* fp)
{
  if (_cache) _cache->print_stats(fp);
}

CachedExprEnvelope * CachedExprEnvelope::check_hit (string & szName, const string& szValue)
{
   CachedExprEnvelope * pRet = 0; 

   if (!_cache)
   {
	_cache.reset(new ClassAdCache());
   }
   pCacheData cache_check = _cache->cache( szName, szValue, 0);
   return 0;

   if (cache_check)
   {
     pRet = new CachedExprEnvelope();
     pRet->nodeKind = EXPR_ENVELOPE;
     pRet->m_pLetter = cache_check;
   }

   return pRet;
}

void CachedExprEnvelope::getAttributeName(std::string & szFillMe)
{
    if (m_pLetter)
    {
        CacheKey *key = m_pLetter->getKey();
        szFillMe.reserve(key->m_length);
        szFillMe.replace(0, key->m_length, key->m_data);
    }
}

ExprTree * CachedExprEnvelope::get()
{
	ExprTree * pRet = 0;
	
	if (m_pLetter)
	{
		//pRet = m_pLetter->pData;
	}
	
	return ( pRet );
}

std::string CachedExprEnvelope::getPacked()
{
	std::string results;
	if (!m_pLetter /*|| m_pLetter->pData*/) return results;
	char * key_data = m_pLetter->m_key.m_data;
	unsigned off = m_pLetter->m_key.m_offset;
	key_data += off;
	unsigned length = m_pLetter->m_key.m_length - m_pLetter->m_key.m_offset;
	results.reserve(length+1); results[length] = '\0';
	results.replace(0, length, key_data, length);
	return results;
}

ExprTree * CachedExprEnvelope::Copy( ) const
{
	CachedExprEnvelope * pRet = new CachedExprEnvelope();
	
	// duplicate as little data as possible.
	pRet->nodeKind = EXPR_ENVELOPE;
	pRet->m_pLetter = this->m_pLetter;
	pRet->parentScope = this->parentScope;
	
	return ( pRet );
}

const ExprTree* CachedExprEnvelope::self() const
{
	return NULL;//m_pLetter->pData;
}

/* This version is for shared-library compatibility.
 * Remove it the next time we have to bump the ClassAds SO version.
 */
const ExprTree* CachedExprEnvelope::self()
{
	return NULL; //m_pLetter->pData;
}

void CachedExprEnvelope::_SetParentScope( const ClassAd* )
{
	// nothing to do here already set @ base
}

bool CachedExprEnvelope::SameAs(const ExprTree* tree) const
{
	bool bRet = false;
	/*
	if (tree && m_pLetter && m_pLetter->pData)
	{
		bRet = m_pLetter->pData->SameAs(((ExprTree*)tree)->self()) ;
	}*/

	return bRet;
}

bool CachedExprEnvelope::isClassad( ClassAd ** ptr )
{
	bool bRet = false;
	
	/*if (m_pLetter && m_pLetter->pData && CLASSAD_NODE == m_pLetter->pData->GetKind() )
	{
		if (ptr)
		{
			*ptr = (ClassAd *) m_pLetter->pData;
		}
		bRet = true;
	}*/
	
	return bRet;
}


bool CachedExprEnvelope::_Evaluate( EvalState& st, Value& v ) const
{
	bool bRet = false;
	
	/*if (m_pLetter && m_pLetter->pData)
	{
		bRet = m_pLetter->pData->Evaluate(st,v);
	}*/

	return bRet;
}

bool CachedExprEnvelope::_Evaluate( EvalState& st, Value& v, ExprTree*& t) const
{
	bool bRet = false;
	
	/*if (m_pLetter && m_pLetter->pData)
	{
		bRet = m_pLetter->pData->Evaluate(st,v,t);
	}*/

	return bRet;
	
}

bool CachedExprEnvelope::_Flatten( EvalState& st, Value& v, ExprTree*& t, int* i)const
{
	bool bRet = false;
	
	/*if (m_pLetter && m_pLetter->pData)
	{
		bRet = m_pLetter->pData->Flatten(st,v,t,i);
	}*/

	return bRet;
	
}





