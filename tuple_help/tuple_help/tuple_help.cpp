#include <iostream>
#include <tuple>
#include <type_traits>

//得到元组的整形索引序列
template <int...>
struct IndexTuple {};

template <int N, int...Indexs>
struct MakeIndexes : MakeIndexes<N - 1, N - 1, Indexs...> {};

template <int...Indexs>
struct MakeIndexes<0, Indexs...>
{
	using  type = IndexTuple<Indexs...>;
};


/* 1.打印tuple的两种方法*/

// (1)通过模板特化和递归来展开并打印tuple
template<class Tuple, std::size_t N>
struct TuplePrinter
{
	static void print(const Tuple& t)
	{
		TuplePrinter<Tuple, N - 1>::print(t);
		std::cout << ", " << std::get<N - 1>(t);
	}
};

//模板特化作为递归终结函数
template<class Tuple>
struct TuplePrinter<Tuple, 1>
{
	static void print(const Tuple& t)
	{
		std::cout << std::get<0>(t);
	}
};

//把元组展开可变模板参数,并且通过可变模板参数得到元组参数大小
template<class... Args>
void PrintTuple(const std::tuple<Args...>& t)
{
	TuplePrinter<decltype(t), sizeof...(Args)>::print(t);
	std::cout << "\n";
}


// (2)通过生成索引序列打印tuple

template <typename T>
void Print(T t)
{
	std::cout << t << std::endl;
}

template <typename T, typename ...Args>
void Print(T t, Args...args)
{
	std::cout << t << ", ";
	Print(args...);
}

template <int...Indexs, typename Tuple>
void Transform(const IndexTuple<Indexs...>& in, Tuple& tp)
{
	Print(std::get<Indexs>(tp)...);
}

/* 2.查找元组指定值的索引*/


//对于可转换的类型比较，if constexpr是c++17的办法，老版本用enable_if重载函数
template <typename T, typename U>
typename std::enable_if<std::is_convertible<T, U>::value ||
	std::is_convertible<U, T>::value, bool>::type
	compare(T t, U u)
{
	if constexpr (std::is_same<T, std::string>::value && std::is_same<U, const char*>::value)
	{
		return t == std::string(u);
	}
	else if constexpr (std::is_same<T, const char*>::value && std::is_same<U, char*>::value)
	{
		return strcmp(t, u) == 0 ? true : false;
	}
	else if constexpr (std::is_same<T, const char*>::value && std::is_same<U, const char*>::value)
	{
		return strcmp(t, u) == 0 ? true : false;
	}
	else
		return t == u;
}

//不能互相转换的则直接返回false
bool compare(...)
{
	return false;
}


//根据值查找索引
template<int I, typename T, typename... Args>
struct find_index
{
	static int call(std::tuple<Args...> const& t, T&& val)
	{
		return (compare(std::get<I - 1>(t), val) ? I - 1 :
			find_index<I - 1, T, Args...>::call(t, std::forward<T>(val)));
	}
};

template<typename T, typename... Args>
struct find_index<0, T, Args...>
{
	static int call(std::tuple<Args...> const& t, T&& val)
	{
		return compare(std::get<0>(t), val) ? 0 : -1;
	}
};

template<typename T, typename... Args>
int find_index_help(std::tuple<Args...> const& t, T&& val)
{
	return find_index<sizeof...(Args), T, Args...>::call(t, std::forward<T>(val));
}

/* 3.打印指定索引的元组数据 */

template <typename Arg>
void GetArgByIndex(int index, std::tuple<Arg>& tp)
{
	std::cout << std::get<0>(tp) << " ";
}

template <typename Arg, typename... Args>
void GetArgByIndex(int index, std::tuple<Arg, Args...>& tp)
{
	if (index < 0 || index >= std::tuple_size<std::tuple<Arg, Args...>>::value)
	{
		throw std::invalid_argument("index is not valid");
	}

	if (index > 0)
	{
		GetArgByIndex(index - 1, (std::tuple<Args...>&) tp);
	}
	else
	{
		std::cout << std::get<0>(tp) << " ";
	}
}

/* 4. 反转元组*/
template<int I, typename IndexTuple, typename... Types>
struct make_indexes_reverse_impl;

//declare
template<int I, int... Indexes, typename T, typename... Types>
struct make_indexes_reverse_impl<I, IndexTuple<Indexes...>, T, Types...>
{
	using type = typename make_indexes_reverse_impl<I - 1, IndexTuple<Indexes..., I - 1>, Types...>::type;
};

//terminate
template<int I, int... Indexes>
struct make_indexes_reverse_impl<I, IndexTuple<Indexes...>>
{
	using type = IndexTuple<Indexes...>;
};

//type trait
template<typename ... Types>
struct make_reverse_indexes : make_indexes_reverse_impl<sizeof...(Types), IndexTuple<>, Types...>
{};

//反转
template <class...Args, int...Indexes>
auto reverse_impl(std::tuple<Args...>&& tup, IndexTuple<Indexes...>&&) ->
decltype(std::make_tuple(std::get<Indexes>(std::forward<std::tuple<Args...>>(tup))...))
{
	return std::make_tuple(std::get<Indexes>(std::forward<std::tuple<Args...>>(tup))...);
}

template <class...Args>
auto Reverse(std::tuple<Args...>&& tup) ->
decltype(reverse_impl(std::forward<std::tuple<Args...>>(tup),
	typename make_reverse_indexes<Args...>::type()))
{
	return reverse_impl(std::forward<std::tuple<Args...>>(tup),
		typename make_reverse_indexes<Args...>::type());
}


/* 5. 将tuple转换为函数参数*/
template <typename F, typename Tuple, int...Indexes>
auto apply(F&& f, IndexTuple<Indexes...>&& in, Tuple&& tp) ->
decltype(std::forward<F>(f)(std::get<Indexes>(tp)...))
{
	std::forward<F>(f)(std::get<Indexes>(tp)...);
}

void TestF(int a, double b)
{
	std::cout << a + b << std::endl;
}

/* 6. 将tuple一一对应合并成pair*/
namespace details
{
	template <int...>
	struct IndexTuple {};

	template <int N, int...Indexs>
	struct MakeIndexes : MakeIndexes<N - 1, N - 1, Indexs...> {};

	template <int...Indexs>
	struct MakeIndexes<0, Indexs...>
	{
		typedef IndexTuple<Indexs...> type;
	};

	template<std::size_t N, typename T1, typename T2>
	using pair_type = std::pair<typename std::tuple_element<N, T1>::type, typename std::tuple_element<N, T2>::type>;

	template<std::size_t N, typename T1, typename T2>
	pair_type<N, T1, T2> pair(const T1& tup1, const T2& tup2)
	{
		//
		return std::make_pair(std::get<N>(tup1), std::get<N>(tup2));
	}

	template<int... Indexes, typename T1, typename T2>
	auto pairs_helper(IndexTuple<Indexes...>, const T1& tup1, const T2& tup2) -> decltype(std::make_tuple(pair<Indexes>(tup1, tup2)...))
	{
		return std::make_tuple(pair<Indexes>(tup1, tup2)...);
	}

} // namespace details

template<typename Tuple1, typename Tuple2>
auto Zip(Tuple1 tup1, Tuple2 tup2) -> decltype(details::pairs_helper(
	typename details::MakeIndexes<std::tuple_size<Tuple1>::value>::type(), tup1, tup2))
{
	static_assert(std::tuple_size<Tuple1>::value == std::tuple_size<Tuple2>::value,
		"tuples should be the same size.");
	return details::pairs_helper(typename
		details::MakeIndexes<std::tuple_size<Tuple1>::value>::type(), tup1, tup2);
}

int main()
{
	auto tp1 = std::make_tuple(1, 2, 3, "abc");
	PrintTuple(tp1);

	Transform(MakeIndexes<std::tuple_size<decltype(tp1)>::value>::type(), tp1);

	int index = find_index_help(tp1, "abc");
	std::cout << index << std::endl;

	constexpr int length = std::tuple_size<decltype(tp1)>::value;
	for (size_t i = 0; i < length; ++i)
	{
		GetArgByIndex(i, tp1);
	}
	std::cout << "\n";

	auto tp2 = std::make_tuple<int, short, double, char>(1, 2, 2.5, 'a');
	auto tp3 = Reverse(std::make_tuple<int, short, double, char>(1, 2, 2.5, 'a'));

	PrintTuple(tp2);
	PrintTuple(tp3);

	apply(TestF, MakeIndexes<2>::type(), std::make_tuple(1, 2));  //输出 : 3

	auto tp4 = std::make_tuple<int, short, double, char>(1, 2, 2.5, 'a');
	auto tp5 = std::make_tuple<double, short, double, char>(1.5, 2, 2.5, 'z');
	auto mypairs = Zip(tp4, tp5);

	system("pause");
	return 0;
}
