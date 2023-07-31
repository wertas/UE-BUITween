
#pragma once

#include "CoreMinimal.h"
#include <type_traits>


namespace BUIDetails
{

// local_storage copied from https://github.com/boost-ext/te (at commit 27465847fe489d33a91014488284210f183cb502)
// 
// Copyright (c) 2018-2019 Kris Jusiak (kris at jusiak dot net)
// 
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
template <std::size_t Size, std::size_t Alignment = 8>
struct local_storage
{
    using mem_t = std::aligned_storage_t<Size, Alignment>;

    template <
        class T,
        class T_ = std::decay_t<T>,
        std::enable_if_t<!std::is_same_v<T_,local_storage>, bool> = true
    >
    constexpr explicit local_storage(T &&t) noexcept(std::is_nothrow_constructible_v<T_,T&&>)
        : ptr{new (&data) T_{std::forward<T>(t)}},
        del{[](mem_t& self) {
        reinterpret_cast<T_ *>(&self)->~T_();
    }},
        copy{[](mem_t& self, const void* other) -> void* {
        if constexpr(std::is_copy_constructible_v<T_>)
            return new (&self) T_{*reinterpret_cast<const T_ *>(other)};
        else
            throw std::runtime_error("local_storage : erased type is not copy constructible");
    }},
        move{[](mem_t& self, void* other) -> void* {
        if constexpr(std::is_move_constructible_v<T_>)
            return new (&self) T_{std::move(*reinterpret_cast<T_ *>(other))};
        else
            throw std::runtime_error("local_storage : erased type is not move constructible");
    }}
    {
        static_assert(sizeof(T_) <= Size, "insufficient size");
        static_assert(Alignment % alignof(T_) == 0, "bad alignment");
    }

    constexpr local_storage(const local_storage& other)
        : ptr{other.ptr ? other.copy(data, other.ptr) : nullptr},
        del{other.del},
        copy{other.copy},
        move{other.move}
    {
    }

    constexpr local_storage& operator=(const local_storage& other)
    {
        if (other.ptr != ptr) {
            reset();
            ptr   = other.ptr ? other.copy(data, other.ptr) : nullptr;
            del   = other.del;
            copy  = other.copy;
            move  = other.move;
        }
        return *this;
    }

    constexpr local_storage(local_storage&& other)
        : ptr{other.ptr ? other.move(data, other.ptr) : nullptr},
        del{other.del},
        copy{other.copy},
        move{other.move}
    {
    }

    constexpr local_storage& operator=(local_storage&& other)
    {
        if (other.ptr != ptr) {
            reset();
            ptr   = other.ptr ? other.move(data, other.ptr) : nullptr;
            del   = other.del;
            copy  = other.copy;
            move  = other.move;
        }
        return *this;
    }

    ~local_storage()
    {
        reset();
    }

    constexpr void reset() noexcept
    {
        if (ptr)
            del(data);
        ptr = nullptr;
    }

    mem_t data;
    void* ptr                               = nullptr;
    void  (*del)(mem_t&)                    = nullptr;
    void* (*copy)(mem_t& mem, const void*)  = nullptr;
    void* (*move)(mem_t& mem, void*)        = nullptr;
};



template <typename Func>
struct FunctionArgs;

template <typename Ret, typename Class, typename... Args>
struct FunctionArgs<Ret(Class::*)(Args...)> {               // pointer to class method
    using type = std::tuple<Ret, Args...>;
    static constexpr bool bIsConst = false;
};

template <typename Ret, typename Class, typename... Args>
struct FunctionArgs<Ret(Class::*)(Args...) const> {         // pointer to const class method
    using type = std::tuple<Ret, Args...>;
    static constexpr bool bIsConst = true;
};

template <typename Func1, typename Func2>
constexpr bool CheckFunctionsSame() {
    return std::is_same_v<typename FunctionArgs<Func1>::type, typename FunctionArgs<Func2>::type>
        && FunctionArgs<Func1>::bIsConst == FunctionArgs<Func2>::bIsConst;
}

template<typename T>
void SwapAndPop(TArray<T>& Vec, typename TArray<T>::SizeType Index)
{
    if constexpr (std::is_signed_v<decltype(Index)>)
    {
        check(Index >= 0);
    }
    check(Index < Vec.Num());

    Vec.RemoveAtSwap(Index, 1, false);
}

class TypeIndexer
{
public:
    using TypeIndex = uint32;

    template<typename T>
    static TypeIndex GetTypeIndex()
    {
        static TypeIndex Value = GetNextNumber();
        return Value;
    }

private:

    static TypeIndex GetNextNumber()
    {
        static TypeIndex Number = -1;
        return ++Number;
    }
};

} // namespace BUIDetails

/*
* TODO: Repair docstring
* Manager that can register classes with void Apply(const float) function and apply them all in Update function
* Classes are moved (or copied) to the pools, each of pools are arrays of actual objects (cache friendly)
*
* Example usage:

float GLOBAL_VALUE = 11.f;

struct AddToGlobal1
{
    void Apply(const float Value) { GLOBAL_VALUE += ValueToAdd; }
    float ValueToAdd;
};

struct AddToGlobal2
{
    void Apply(const float Value) { GLOBAL_VALUE += Value;}
};

FManager Manager;
Manager.Update(5.5f);                       // +0
std::cout << GLOBAL_VALUE_1 << '\n';        // 11
Manager.Emplace<AddToGlobal1>(2.f);
std::cout << GLOBAL_VALUE_1 << '\n';        // 11
Manager.Update(5.5f);                       // +2
std::cout << GLOBAL_VALUE_1 << '\n';        // 13
Manager.Add(AddToGlobal2{});
Manager.Update(5.5f);                       // +2 + 5.5 = +7.5
std::cout << GLOBAL_VALUE_1 << std::endl;   // 20.5
return 0;


*/



class FBUIPoolManager
{
public:

    using EntityHandle = uint32;
    static const EntityHandle ALL_ENTITIES = 0;
    static const EntityHandle INVALID_ENTITY = 1; // Must be the last used reserved number

    // All objects that are going to be added to the manager must have this functions implemented
    struct ExampleObject
    {
        void Apply() {}
        void Begin() {}
        void SetActive(const bool) {}
        void SetData(void*) {}
    };

    // ********** External interface BEGIN **********

    void Apply(const EntityHandle Handle)
    {
        for (auto& Pool: Pools)
        {
            Pool.ApplyEntity_OrAll(Handle);
        }
    }

    void ApplyAll()
    {
        Apply(ALL_ENTITIES);
    }

    void EntityBegin(const EntityHandle Handle)
    {
        for (auto& Pool: Pools)
        {
            Pool.EntityBegin_OrAll(Handle);
        }
    }

    void RemoveEntity(const EntityHandle Handle)
    {
        for (auto& Pool: Pools)
        {
            Pool.RemoveEntity_OrAll(Handle);
        }
    }

    void RemoveAll()
    {
        RemoveEntity(ALL_ENTITIES);

        TypeToPoolIndexMap.Empty();
        Pools.Empty();
        // LastEntity = 0;
    }

    void SetEntityActive(const EntityHandle Handle, const bool bValue)
    {
        for (auto& Pool: Pools)
        {
            Pool.SetEntityActive_OrAll(Handle, bValue);
        }
    }

    void SetActiveAllEntities(const bool bValue)
    {
        SetEntityActive(ALL_ENTITIES, bValue);
    }

    void SetEntityData(const EntityHandle Handle, void* Data)
    {
        for (auto& Pool: Pools)
        {
            Pool.SetEntityData(Handle, Data);
        }
    }

    // ********** External interface END **********

    EntityHandle GetEntityHandle()
    {
        ++LastEntity;
        if (LastEntity == 0)
        {
            LastEntity = INVALID_ENTITY + 1;
        }
        return LastEntity;
    }

    template<typename T>
    void Add(const EntityHandle Entity, T&& Object)
    {
        check(!Pools[GetPoolIndex<T>()].template IsEntityPresentInPool_ItsAnError<T>(Entity));

        Pools[GetPoolIndex<T>()].template EmplaceObject<T>(Entity, std::forward<T>(Object));
    }

    template<typename T, typename ...Args>
    void Emplace(const EntityHandle Entity, Args&&... args)
    {
        check(!Pools[GetPoolIndex<T>()].template IsEntityPresentInPool_ItsAnError<T>(Entity));

        Pools[GetPoolIndex<T>()].template EmplaceObject<T>(Entity, std::forward<Args>(args)...);
    }

private:

    struct VirtualTable
    {
        virtual ~VirtualTable() noexcept = default;

        virtual void ApplyEntity_OrAll(const EntityHandle Handle) noexcept = 0;
        virtual void EntityBegin_OrAll(const EntityHandle Handle) noexcept = 0;
        virtual void RemoveEntity_OrAll(const EntityHandle Handle) noexcept = 0;
        virtual void SetEntityActive_OrAll(const EntityHandle Handle, const bool bValue) noexcept = 0;
        virtual void SetEntityData(const EntityHandle Handle, void* Data) noexcept = 0;
    };

    template< typename T >
    class PoolImpl final : VirtualTable
    {
    public:

        static_assert(BUIDetails::CheckFunctionsSame<decltype(&std::remove_reference_t<T>::Apply), decltype(&ExampleObject::Apply)>(), "Object must have void Apply() function");
        static_assert(BUIDetails::CheckFunctionsSame<decltype(&std::remove_reference_t<T>::Begin), decltype(&ExampleObject::Begin)>(), "Object must have void Begin() function");
        static_assert(BUIDetails::CheckFunctionsSame<decltype(&std::remove_reference_t<T>::SetActive), decltype(&ExampleObject::SetActive)>(), "Object must have void SetActive(const bool) function");
        static_assert(BUIDetails::CheckFunctionsSame<decltype(&std::remove_reference_t<T>::SetData), decltype(&ExampleObject::SetData)>(), "Object must have void SetData(void*) function");

        // ********** Internal interface implementation BEGIN **********

        virtual void ApplyEntity_OrAll(const EntityHandle Handle) noexcept override
        {
            if (Handle == ALL_ENTITIES)
            {
                for (auto& Object : Objects)
                {
                    Object.Apply();
                }
                return;
            }

            if (auto* IndexPtr = HandleToIndexMap.Find(Handle))
            {
                Objects[*IndexPtr].Apply();
            }
        }

        virtual void EntityBegin_OrAll(const EntityHandle Handle) noexcept override
        {
            if (Handle == ALL_ENTITIES)
            {
                for (auto& Object : Objects)
                {
                    Object.Begin();
                }
                return;
            }

            if (auto* IndexPtr = HandleToIndexMap.Find(Handle))
            {
                Objects[*IndexPtr].Begin();
            }
        }

        virtual void RemoveEntity_OrAll(const EntityHandle Handle) noexcept override
        {
            if (Handle == ALL_ENTITIES)
            {
                Objects.Empty(Objects.GetSlack());
                Handles.Empty(Handles.GetSlack());
                HandleToIndexMap.Empty();
                return;
            }

            if (auto* IndexPtr = HandleToIndexMap.Find(Handle))
            {
				if (Objects.Num() == 1)
				{
					check(1 == Handles.Num());
					check(1 == HandleToIndexMap.Num());
                    check(*IndexPtr == 0);
					check(Handles[0] == Handle);
					Objects.Empty(Objects.GetSlack());
					Handles.Empty(Handles.GetSlack());
					HandleToIndexMap.Empty();
					return;
				}

                auto Index = *IndexPtr;                                         // Get index in array

				check(Index < Objects.Num());

                const bool bNeedToFixLastEntityInMap = Objects.Num() - 1 != Index; // Last element was already removed, so no need to fix if false

                BUIDetails::SwapAndPop(Objects, Index);
                BUIDetails::SwapAndPop(Handles, Index);

                if (bNeedToFixLastEntityInMap)
                {
                    EntityHandle HandleToFix = Handles[Index];                  // Retrieve handle that became invalid after swap
                    auto* IndexToFixPtr = HandleToIndexMap.Find(HandleToFix);
                    check(IndexToFixPtr);                                       // TODO: maybe decide to make runtime check??? This must always be false by design
                    *IndexToFixPtr = Index;                                     // Find handle for swapped element and reassign to remain valid
                }
                HandleToIndexMap.Remove(Handle);                                // Cleanup removed handle
            }
        }

        virtual void SetEntityActive_OrAll(const EntityHandle Handle, const bool bValue) noexcept override
        {
            if (Handle == ALL_ENTITIES)
            {
                for (auto& Object : Objects)
                {
                    Object.SetActive(bValue);
                }
                return;
            }

            if (auto* IndexPtr = HandleToIndexMap.Find(Handle))
            {
                Objects[*IndexPtr].SetActive(bValue);
            }
        }

        virtual void SetEntityData(const EntityHandle Handle, void* Data) noexcept override
        {
            if (auto* IndexPtr = HandleToIndexMap.Find(Handle))
            {
                Objects[*IndexPtr].SetData(Data);
            }
        }

        // ********** Internal interface implementation END **********

        template< typename T, typename ...Args >
        void EmplaceObject(const EntityHandle Entity, Args&&... args)
        {
            const auto* ExistingObjectIndexPtr = HandleToIndexMap.Find(Entity);
            if (ExistingObjectIndexPtr)
            {
                // Replace existing object for this entity, if exists
                Objects[*ExistingObjectIndexPtr] = T{ std::forward<Args>(args)... };
            }
            else
            {
                HandleToIndexMap.Add(Entity, Objects.Num());
                Objects.Emplace(std::forward<Args>(args)...);
                Handles.Emplace(Entity);
                check(Objects.Num() == Handles.Num());
            }
        }

        template< typename T >
        bool IsEntityPresentInPool_ItsAnError(const EntityHandle Entity)
        {
            return HandleToIndexMap.Find(Entity) != nullptr;
        }

    private:

        using ArrayT = TArray<std::remove_reference_t<T>>;

        TMap<EntityHandle, typename ArrayT::SizeType> HandleToIndexMap;
        TArray<EntityHandle> Handles;
        ArrayT Objects;
    };

    class PoolInterface
    {
    public:
        using FStorage = BUIDetails::local_storage<sizeof(PoolImpl<ExampleObject>)>;

        PoolInterface(PoolInterface&& Other) = default;
        PoolInterface& operator=(PoolInterface&& Other) = default;

        template<class T, class T_ = std::decay_t<T>, std::enable_if_t<!std::is_same<T_, PoolInterface>::value, bool> = true>
        PoolInterface(T&& obj)
            : Storage{std::forward<T>(obj)}
        {
        }

        // ********** Internal interface BEGIN **********

        void ApplyEntity_OrAll(const EntityHandle Handle) noexcept
        {
            GetTable()->ApplyEntity_OrAll(Handle);
        }

        void EntityBegin_OrAll(const EntityHandle Handle) noexcept
        {
            GetTable()->EntityBegin_OrAll(Handle);
        }

        void RemoveEntity_OrAll(const EntityHandle Handle) noexcept
        {
            GetTable()->RemoveEntity_OrAll(Handle);
        }

        void SetEntityActive_OrAll(const EntityHandle Handle, const bool bValue)
        {
            GetTable()->SetEntityActive_OrAll(Handle, bValue);
        }

        void SetEntityData(const EntityHandle Handle, void* Data)
        {
            GetTable()->SetEntityData(Handle, Data);
        }

        // ********** Internal interface END **********

        template< typename T, typename ...Args >
        void EmplaceObject(const EntityHandle Entity, Args&&... args )
        {
            auto& StorageImpl = GetStorage<PoolImpl<T>>(Storage);
            StorageImpl.EmplaceObject<T>(Entity, std::forward<Args>(args)...);
        }

        template< typename T >
        bool IsEntityPresentInPool_ItsAnError(const EntityHandle Entity)
        {
            auto& StorageImpl = GetStorage<PoolImpl<T>>(Storage);
            return StorageImpl.IsEntityPresentInPool_ItsAnError<T>(Entity);
        }

    private:

        template<typename T>
        static auto& GetStorage(FStorage& Storage)
        {
            return *static_cast<T*>(Storage.ptr);
        }

        VirtualTable* GetTable() noexcept
        {
            return static_cast<VirtualTable*>(Storage.ptr);
        }

    private:

        FStorage Storage;
    };

    using PoolsArray = TArray<PoolInterface>;
    using TypeIndex = BUIDetails::TypeIndexer::TypeIndex;

    template<typename T>
    PoolsArray::SizeType GetPoolIndex()
    {
        const TypeIndex TypeId = BUIDetails::TypeIndexer::GetTypeIndex<T>();
        if (PoolsArray::SizeType* Idx = TypeToPoolIndexMap.Find(TypeId))
        {
            return *Idx;
        }
        else
        {
            const PoolsArray::SizeType Index = Pools.Num();
            Pools.Emplace(PoolImpl<T>{});
            TypeToPoolIndexMap.Add({TypeId, Index});
            return Index;
        }
        static_assert(sizeof(PoolImpl<ExampleObject>) == sizeof(PoolImpl<T>));
    }

private:

    EntityHandle LastEntity = INVALID_ENTITY;

    TMap<TypeIndex, PoolsArray::SizeType> TypeToPoolIndexMap;
    TArray<PoolInterface> Pools;
};
