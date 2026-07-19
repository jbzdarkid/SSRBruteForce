using System;

// Token: 0x02000103 RID: 259
public struct Pair<T>
{
	// Token: 0x06000D46 RID: 3398 RVA: 0x0005339F File Offset: 0x0005179F
	public Pair(T _x, T _y)
	{
		this.__x = _x;
		this.__y = _y;
	}

	// Token: 0x170003FF RID: 1023
	// (get) Token: 0x06000D47 RID: 3399 RVA: 0x000533AF File Offset: 0x000517AF
	public T x
	{
		get
		{
			return this.__x;
		}
	}

	// Token: 0x17000400 RID: 1024
	// (get) Token: 0x06000D48 RID: 3400 RVA: 0x000533B7 File Offset: 0x000517B7
	public T y
	{
		get
		{
			return this.__y;
		}
	}

	// Token: 0x06000D49 RID: 3401 RVA: 0x000533C0 File Offset: 0x000517C0
	public override string ToString()
	{
		string[] array = new string[5];
		array[0] = "(";
		int num = 1;
		T x = this.x;
		array[num] = x.ToString();
		array[2] = ",";
		int num2 = 3;
		T y = this.y;
		array[num2] = y.ToString();
		array[4] = ")";
		return string.Concat(array);
	}

	// Token: 0x06000D4A RID: 3402 RVA: 0x00053420 File Offset: 0x00051820
	public bool Equals(Pair<T> other)
	{
		T x = this.x;
		bool flag;
		if (x.Equals(other.x))
		{
			T y = this.y;
			flag = y.Equals(other.y);
		}
		else
		{
			flag = false;
		}
		return flag;
	}

	// Token: 0x040009E2 RID: 2530
	private readonly T __x;

	// Token: 0x040009E3 RID: 2531
	private readonly T __y;
}
