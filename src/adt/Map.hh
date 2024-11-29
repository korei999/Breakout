#pragma once

#include "Vec.hh"
#include "hash.hh"

namespace adt
{

constexpr f32 MAP_DEFAULT_LOAD_FACTOR = 0.5f;
constexpr f32 MAP_DEFAULT_LOAD_FACTOR_INV = 1.0f / MAP_DEFAULT_LOAD_FACTOR;

enum class MAP_RESULT_STATUS : u8 { FOUND, NOT_FOUND, INSERTED };

template<typename K, typename V>
struct KeyValue
{
    K key {};
    V val {};
};

template<typename K, typename V>
struct MapBucket
{
    KeyValue<K, V> keyValue {};
    bool bOccupied = false;
    bool bDeleted = false;
};

/* custom return type for insert/search operations */
template<typename K, typename V>
struct MapResult
{
    KeyValue<K, V>* pData {};
    u64 hash {};
    MAP_RESULT_STATUS eStatus {};

    constexpr explicit operator bool() const
    {
        return this->pData != nullptr;
    }
};

template<typename K, typename V> struct MapBase;

template<typename K, typename V>
inline u32 MapIdx(MapBase<K, V>* s, KeyValue<K, V>* p);

template<typename K, typename V>
inline u32 MapIdx(MapBase<K, V>* s, MapResult<K, V> res);

template<typename K, typename V>
inline u32 MapFirstI(MapBase<K, V>* s);

template<typename K, typename V>
inline u32 MapNextI(MapBase<K, V>* s, u32 i);

template<typename K, typename V>
inline f32 MapLoadFactor(MapBase<K, V>* s);

template<typename K, typename V>
inline MapResult<K, V> MapInsert(MapBase<K ,V>* s, Allocator* p, const KeyValue<K, V>& kv);

template<typename K, typename V>
[[nodiscard]] inline MapResult<K, V> MapSearch(MapBase<K, V>* s, const K& key);

template<typename K, typename V>
inline void MapRemove(MapBase<K, V>*s, u32 i);

template<typename K, typename V>
inline void MapRemove(MapBase<K, V>*s, const K& key);

template<typename K, typename V>
inline MapResult<K, V> MapTryInsert(MapBase<K, V>* s, Allocator* p, const KeyValue<K, V> kv);

template<typename K, typename V>
inline void MapDestroy(MapBase<K, V>* s, Allocator* p);

template<typename K, typename V>
inline u32 MapCap(MapBase<K, V>* s);

template<typename K, typename V>
inline u32 MapSize(MapBase<K, V>* s);

template<typename K, typename V>
inline void _MapRehash(MapBase<K, V>* s, Allocator* p, u32 size);

template<typename K, typename V>
struct MapBase
{
    VecBase<MapBucket<K, V>> aBuckets {};
    f32 maxLoadFactor {};
    u32 nOccupied {};

    MapBase() = default;
    MapBase(Allocator* pAllocator, u32 prealloc = SIZE_MIN);

    struct It
    {
        MapBase* s {};
        u32 i = 0;

        It(MapBase* _s, u32 _i) : s(_s), i(_i) {}

        KeyValue<K, V>& operator*() { return s->aBuckets[i].keyValue; }
        KeyValue<K, V>* operator->() { return &s->aBuckets[i].keyValue; }

        It operator++()
        {
            i = MapNextI(s, i);
            return {s, i};
        }
        It operator++(int) { KeyValue<K, V>* tmp = s++; return tmp; }

        friend bool operator==(const It& l, const It& r) { return l.i == r.i; }
        friend bool operator!=(const It& l, const It& r) { return l.i != r.i; }
    };

    It begin() { return {this, MapFirstI(this)}; }
    It end() { return {this, NPOS}; }

    const It begin() const { return {this, MapFirstI(this)}; }
    const It end() const { return {this, NPOS}; }
};

template<typename K, typename V>
inline u32
MapIdx(MapBase<K, V>* s, KeyValue<K, V>* p)
{
    auto r = (MapBucket<K, V>*)p - &s->aBuckets[0];
    assert(r < VecCap(&s->aBuckets));
    return r;
}

template<typename K, typename V>
inline u32
MapIdx(MapBase<K, V>* s, MapResult<K, V> res)
{
    return MapIdx(s, res.pData);
}

template<typename K, typename V>
inline u32
MapFirstI(MapBase<K, V>* s)
{
    u32 i = 0;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied)
        ++i;

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename K, typename V>
inline u32
MapNextI(MapBase<K, V>* s, u32 i)
{
    do ++i;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied);

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename K, typename V>
inline f32
MapLoadFactor(MapBase<K, V>* s)
{
    return f32(s->nOccupied) / f32(VecCap(&s->aBuckets));
}

template<typename K, typename V>
inline MapResult<K, V>
MapInsert(MapBase<K ,V>* s, Allocator* p, const KeyValue<K, V>& kv)
{
    if (VecCap(&s->aBuckets) == 0) *s = {p};

    if (MapLoadFactor(s) >= s->maxLoadFactor)
        _MapRehash(s, p, VecCap(&s->aBuckets) * 2);

    u64 hash = hash::func(kv.key);
    u32 idx = u32(hash % VecCap(&s->aBuckets));

    while (s->aBuckets[idx].bOccupied)
    {
        ++idx;
        if (idx >= VecCap(&s->aBuckets)) idx = 0;
    }

    s->aBuckets[idx].keyValue = kv;
    s->aBuckets[idx].bOccupied = true;
    s->aBuckets[idx].bDeleted = false;
    ++s->nOccupied;

    return {
        .pData = &s->aBuckets[idx].keyValue,
        .hash = hash,
        .eStatus = MAP_RESULT_STATUS::INSERTED
    };
}

template<typename K, typename V>
[[nodiscard]] inline MapResult<K, V>
MapSearch(MapBase<K, V>* s, const K& key)
{
    MapResult<K, V> res {.eStatus = MAP_RESULT_STATUS::NOT_FOUND};

    if (s->nOccupied == 0) return res;

    auto& aBuckets = s->aBuckets;

    u64 hash = hash::func(key);
    u32 idx = u32(hash % VecCap(&aBuckets));
    res.hash = hash;

    while (aBuckets[idx].bOccupied || aBuckets[idx].bDeleted)
    {
        if (!aBuckets[idx].bDeleted && aBuckets[idx].keyValue.key == key)
        {
            res.pData = &aBuckets[idx].keyValue;
            res.eStatus = MAP_RESULT_STATUS::FOUND;
            break;
        }

        ++idx;
        if (idx >= VecCap(&aBuckets)) idx = 0;
    }

    return res;
}

template<typename K, typename V>
inline void
MapRemove(MapBase<K, V>*s, u32 i)
{
    s->aBuckets[i].bDeleted = true;
    s->aBuckets[i].bOccupied = false;
    --s->nOccupied;
}

template<typename K, typename V>
inline void
MapRemove(MapBase<K, V>*s, const K& key)
{
    auto f = MapSearch(s, key);
    assert(f && "[Map]: not found");
    MapRemove(s, MapIdx(s, f));
}


template<typename K, typename V>
inline MapResult<K, V>
MapTryInsert(MapBase<K, V>* s, Allocator* p, const KeyValue<K, V> kv)
{
    auto f = MapSearch(s, kv.key);
    if (f)
    {
        f.eStatus = MAP_RESULT_STATUS::FOUND;
        return f;
    }
    else return MapInsert(s, p, kv);
}

template<typename K, typename V>
inline void
MapDestroy(MapBase<K, V>* s, Allocator* p)
{
    VecDestroy(&s->aBuckets, p);
}

template<typename K, typename V>
inline u32 MapCap(MapBase<K, V>* s)
{
    return VecCap(&s->aBuckets);
}

template<typename K, typename V>
inline u32 MapSize(MapBase<K, V>* s)
{
    return s->nOccupied;
}

template<typename K, typename V>
inline void
_MapRehash(MapBase<K, V>* s, Allocator* p, u32 size)
{
    auto mNew = MapBase<K, V>(p, size);

    for (u32 i = 0; i < VecCap(&s->aBuckets); ++i)
        if (s->aBuckets[i].bOccupied)
            MapInsert(&mNew, p, s->aBuckets[i].keyValue);

    MapDestroy(s, p);
    *s = mNew;
}

template<typename K, typename V>
MapBase<K, V>::MapBase(Allocator* pAllocator, u32 prealloc)
    : aBuckets(pAllocator, prealloc * MAP_DEFAULT_LOAD_FACTOR_INV),
      maxLoadFactor(MAP_DEFAULT_LOAD_FACTOR)
{
    VecSetSize(&aBuckets, pAllocator, prealloc * MAP_DEFAULT_LOAD_FACTOR_INV);
    memset(aBuckets.pData, 0, sizeof(aBuckets[0]) * VecSize(&aBuckets));
}

template<typename K, typename V>
struct Map
{
    MapBase<K, V> base {};
    Allocator* pA {};

    Map() = default;
    Map(Allocator* _pA, u32 prealloc = SIZE_MIN)
        : base(_pA, prealloc), pA(_pA) {}

    MapBase<K, V>::It begin() { return base.begin(); }
    MapBase<K, V>::It end() { return base.end(); }
    MapBase<K, V>::It rbegin() { return base.rbegin(); }
    MapBase<K, V>::It rend() { return rend(); }

    const MapBase<K, V>::It begin() const { return base.begin(); }
    const MapBase<K, V>::It end() const { return base.end(); }
    const MapBase<K, V>::It rbegin() const { return base.rbegin(); }
    const MapBase<K, V>::It rend() const { return rend(); }
};

template<typename K, typename V>
inline u32 MapIdx(Map<K, V>* s, KeyValue<K, V>* pItem) { return MapIdx(&s->base, pItem); }

template<typename K, typename V>
inline u32 MapIdx(Map<K, V>* s, MapResult<K, V> res) { return MapIdx<K, V>(&s->base, res); }

template<typename K, typename V>
inline u32 MapFirstI(Map<K, V>* s) { return MapFirstI(&s->base); }

template<typename K, typename V>
inline u32 MapNextI(Map<K, V>* s, u32 i) { return MapNextI(&s->base, i); }

template<typename K, typename V>
inline f32 MapLoadFactor(Map<K, V>* s) { return MapLoadFactor(&s->base); }

template<typename K, typename V>
inline MapResult<K, V> MapInsert(Map<K ,V>* s, const KeyValue<K, V>& kv) { return MapInsert(&s->base, s->pA, kv); }

template<typename K, typename V>
[[nodiscard]] inline MapResult<K, V> MapSearch(Map<K, V>* s, const K& key) { return MapSearch(&s->base, key); }

template<typename K, typename V>
inline void MapRemove(Map<K, V>*s, u32 i) { MapRemove(&s->base, i); }

template<typename K, typename V>
inline void MapRemove(Map<K, V>*s, const K& key) { MapRemove(&s->base, key); }

template<typename K, typename V>
inline MapResult<K, V> MapTryInsert(Map<K, V>* s, const KeyValue<K, V> kv) { return MapTryInsert(&s->base, s->pA, kv); }

template<typename K, typename V>
inline void MapDestroy(Map<K, V>* s) { MapDestroy(&s->base, s->pA); }

template<typename K, typename V>
inline u32 MapCap(Map<K, V>* s) { return MapCap(&s->base); }

template<typename K, typename V>
inline u32 MapSize(Map<K, V>* s) { return MapSize(&s->base); }

namespace print
{

inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, MAP_RESULT_STATUS eStatus)
{
    ctx.fmt = "{}";
    ctx.fmtIdx = 0;
    constexpr String map[] {
        "FOUND", "NOT_FOUND", "INSERTED"
    };

    auto statusIdx = std::underlying_type_t<MAP_RESULT_STATUS>(eStatus);
    assert(statusIdx < utils::size(map) && "out of range enum");
    return printArgs(ctx, map[statusIdx]);
}

} /* namespace print */

} /* namespace adt */
