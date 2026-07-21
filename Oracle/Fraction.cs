using System;

// Token: 0x020000F1 RID: 241
[Serializable]
public struct Fraction : IComparable<Fraction>
{
	// Token: 0x06000C75 RID: 3189 RVA: 0x00050A1D File Offset: 0x0004EE1D
	public Fraction(int num)
	{
		this.num = num;
		this.den = 1;
	}

	// Token: 0x06000C76 RID: 3190 RVA: 0x00050A30 File Offset: 0x0004EE30
	public Fraction(int num, int den)
	{
		if (den > 10000)
		{
			int num2 = Fraction.GCD(Math.Abs(num), Math.Abs(den));
			num /= num2;
			den /= num2;
		}
		Fraction.CheckDenominatorZero(den);
		this.num = num;
		this.den = den;
	}

	// Token: 0x06000C77 RID: 3191 RVA: 0x00050A78 File Offset: 0x0004EE78
	public static Fraction operator +(Fraction a, Fraction b)
	{
		int num = a.num * b.den + b.num * a.den;
		int num2 = a.den * b.den;
		return new Fraction(num, num2);
	}

	// Token: 0x06000C78 RID: 3192 RVA: 0x00050ABC File Offset: 0x0004EEBC
	public static Fraction operator +(Fraction a, int b)
	{
		return new Fraction(b * a.den + a.num, a.den);
	}

	// Token: 0x06000C79 RID: 3193 RVA: 0x00050ADB File Offset: 0x0004EEDB
	public static Fraction operator +(int a, Fraction b)
	{
		return new Fraction(a * b.den + b.num, b.den);
	}

	// Token: 0x06000C7A RID: 3194 RVA: 0x00050AFC File Offset: 0x0004EEFC
	public static Fraction operator -(Fraction a, Fraction b)
	{
		int num = a.num * b.den - b.num * a.den;
		int num2 = a.den * b.den;
		return new Fraction(num, num2);
	}

	// Token: 0x06000C7B RID: 3195 RVA: 0x00050B40 File Offset: 0x0004EF40
	public static Fraction operator -(Fraction a, int b)
	{
		return new Fraction(a.num - b * a.den, a.den);
	}

	// Token: 0x06000C7C RID: 3196 RVA: 0x00050B5F File Offset: 0x0004EF5F
	public static Fraction operator -(int a, Fraction b)
	{
		return new Fraction(a * b.den - b.num, b.den);
	}

	// Token: 0x06000C7D RID: 3197 RVA: 0x00050B80 File Offset: 0x0004EF80
	public static Fraction operator *(Fraction a, Fraction b)
	{
		int num = a.num * b.num;
		int num2 = a.den * b.den;
		return new Fraction(num, num2);
	}

	// Token: 0x06000C7E RID: 3198 RVA: 0x00050BB4 File Offset: 0x0004EFB4
	public static Fraction operator *(Fraction a, int b)
	{
		return new Fraction(a.num * b, a.den);
	}

	// Token: 0x06000C7F RID: 3199 RVA: 0x00050BCB File Offset: 0x0004EFCB
	public static Fraction operator *(int a, Fraction b)
	{
		return new Fraction(a * b.num, b.den);
	}

	// Token: 0x06000C80 RID: 3200 RVA: 0x00050BE4 File Offset: 0x0004EFE4
	public static Fraction operator /(Fraction a, Fraction b)
	{
		int num = a.num * b.den;
		int num2 = a.den * b.num;
		if (num2 == 0)
		{
			throw new DivideByZeroException();
		}
		return new Fraction(num, num2);
	}

	// Token: 0x06000C81 RID: 3201 RVA: 0x00050C24 File Offset: 0x0004F024
	public static Fraction operator /(Fraction a, int b)
	{
		return new Fraction(a.num, a.den * b);
	}

	// Token: 0x06000C82 RID: 3202 RVA: 0x00050C3B File Offset: 0x0004F03B
	public static Fraction operator /(int a, Fraction b)
	{
		return new Fraction(a * b.den, b.num);
	}

	// Token: 0x06000C83 RID: 3203 RVA: 0x00050C52 File Offset: 0x0004F052
	public static Fraction operator ^(Fraction a, int n)
	{
		return new Fraction((int)Math.Pow((double)a.num, (double)n), (int)Math.Pow((double)a.den, (double)n));
	}

	// Token: 0x06000C84 RID: 3204 RVA: 0x00050C79 File Offset: 0x0004F079
	public static bool operator ==(Fraction a, Fraction b)
	{
		return a.num * b.den == b.num * a.den;
	}

	// Token: 0x06000C85 RID: 3205 RVA: 0x00050C9B File Offset: 0x0004F09B
	public static bool operator ==(Fraction a, int b)
	{
		return a.num == b * a.den;
	}

	// Token: 0x06000C86 RID: 3206 RVA: 0x00050CAF File Offset: 0x0004F0AF
	public static bool operator ==(int b, Fraction a)
	{
		return a.num == b * a.den;
	}

	// Token: 0x06000C87 RID: 3207 RVA: 0x00050CC3 File Offset: 0x0004F0C3
	public static bool operator !=(Fraction a, Fraction b)
	{
		return a.num * b.den != b.num * a.den;
	}

	// Token: 0x06000C88 RID: 3208 RVA: 0x00050CE8 File Offset: 0x0004F0E8
	public static bool operator !=(int a, Fraction b)
	{
		return !(a == b);
	}

	// Token: 0x06000C89 RID: 3209 RVA: 0x00050CF4 File Offset: 0x0004F0F4
	public static bool operator !=(Fraction a, int b)
	{
		return !(a == b);
	}

	// Token: 0x06000C8A RID: 3210 RVA: 0x00050D00 File Offset: 0x0004F100
	public static bool operator >(Fraction a, int b)
	{
		return a.num > b * a.den;
	}

	// Token: 0x06000C8B RID: 3211 RVA: 0x00050D14 File Offset: 0x0004F114
	public static bool operator >(int a, Fraction b)
	{
		return a * b.den > b.num;
	}

	// Token: 0x06000C8C RID: 3212 RVA: 0x00050D28 File Offset: 0x0004F128
	public static bool operator >(Fraction a, Fraction b)
	{
		return a.num * b.den > b.num * a.den;
	}

	// Token: 0x06000C8D RID: 3213 RVA: 0x00050D4A File Offset: 0x0004F14A
	public static bool operator >=(int a, Fraction b)
	{
		return !(a < b);
	}

	// Token: 0x06000C8E RID: 3214 RVA: 0x00050D56 File Offset: 0x0004F156
	public static bool operator >=(Fraction a, int b)
	{
		return !(a < b);
	}

	// Token: 0x06000C8F RID: 3215 RVA: 0x00050D62 File Offset: 0x0004F162
	public static bool operator >=(Fraction a, Fraction b)
	{
		return !(a < b);
	}

	// Token: 0x06000C90 RID: 3216 RVA: 0x00050D6E File Offset: 0x0004F16E
	public static bool operator <(int a, Fraction b)
	{
		return a * b.den < b.num;
	}

	// Token: 0x06000C91 RID: 3217 RVA: 0x00050D82 File Offset: 0x0004F182
	public static bool operator <(Fraction a, int b)
	{
		return a.num < b * a.den;
	}

	// Token: 0x06000C92 RID: 3218 RVA: 0x00050D96 File Offset: 0x0004F196
	public static bool operator <(Fraction a, Fraction b)
	{
		return a.num * b.den < b.num * a.den;
	}

	// Token: 0x06000C93 RID: 3219 RVA: 0x00050DB8 File Offset: 0x0004F1B8
	public static bool operator <=(Fraction a, Fraction b)
	{
		return !(a > b);
	}

	// Token: 0x06000C94 RID: 3220 RVA: 0x00050DC4 File Offset: 0x0004F1C4
	public static bool operator <=(int a, Fraction b)
	{
		return !(a > b);
	}

	// Token: 0x06000C95 RID: 3221 RVA: 0x00050DD0 File Offset: 0x0004F1D0
	public static bool operator <=(Fraction a, int b)
	{
		return !(a > b);
	}

	// Token: 0x06000C96 RID: 3222 RVA: 0x00050DDC File Offset: 0x0004F1DC
	public int CompareTo(Fraction other)
	{
		return ((float)this).CompareTo((float)other);
	}

	// Token: 0x06000C97 RID: 3223 RVA: 0x00050E02 File Offset: 0x0004F202
	public override string ToString()
	{
		if (this.den == 1)
		{
			return this.num.ToStringFast();
		}
		return this.num.ToStringFast() + "/" + this.den.ToStringFast();
	}

	// Token: 0x06000C98 RID: 3224 RVA: 0x00050E3C File Offset: 0x0004F23C
	public override bool Equals(object o)
	{
		if (o == null || o.GetType() != base.GetType())
		{
			return false;
		}
		Fraction fraction = (Fraction)o;
		return this == fraction;
	}

	// Token: 0x06000C99 RID: 3225 RVA: 0x00050E7F File Offset: 0x0004F27F
	public override int GetHashCode()
	{
		return this.num ^ this.den;
	}

	// Token: 0x06000C9A RID: 3226 RVA: 0x00050E8E File Offset: 0x0004F28E
	public static explicit operator double(Fraction f)
	{
		return (double)f.num / (double)f.den;
	}

	// Token: 0x06000C9B RID: 3227 RVA: 0x00050EA1 File Offset: 0x0004F2A1
	public static explicit operator float(Fraction f)
	{
		return (float)f.num / (float)f.den;
	}

	// Token: 0x06000C9C RID: 3228 RVA: 0x00050EB4 File Offset: 0x0004F2B4
	public static implicit operator Fraction(int value)
	{
		return new Fraction(value);
	}

	// Token: 0x06000C9D RID: 3229 RVA: 0x00050EBC File Offset: 0x0004F2BC
	public static int GCD(int a, int b)
	{
		if (a > b)
		{
			int num = b;
			b = a;
			a = num;
		}
		while (b != 0)
		{
			int num = a % b;
			a = b;
			b = num;
		}
		return a;
	}

	// Token: 0x06000C9E RID: 3230 RVA: 0x00050EEE File Offset: 0x0004F2EE
	private static void CheckDenominatorZero(int den)
	{
		if (den == 0)
		{
			throw new ArithmeticException("The denominator of any fraction cannot have the value zero");
		}
	}

	// Token: 0x06000C9F RID: 3231 RVA: 0x00050F04 File Offset: 0x0004F304
	public static Fraction Parse(string fraction)
	{
		if (fraction == null)
		{
			throw new FormatException();
		}
		string[] array = fraction.Split(new char[] { '/' });
		int num = array.Length;
		if (num == 2)
		{
			int num2 = array[0].IntParseFast();
			int num3 = array[1].IntParseFast();
			return new Fraction(num2, num3);
		}
		if (num == 4)
		{
			int num4 = array[0].IntParseFast();
			int num5 = array[1].IntParseFast();
			Fraction fraction2 = new Fraction(num4, num5);
			int num6 = array[2].IntParseFast();
			int num7 = array[3].IntParseFast();
			Fraction fraction3 = new Fraction(num6, num7);
			return fraction2 / fraction3;
		}
		throw new FormatException();
	}

	// Token: 0x06000CA0 RID: 3232 RVA: 0x00050FA8 File Offset: 0x0004F3A8
	public Fraction Inverse()
	{
		return new Fraction(this.den, this.num);
	}

	// Token: 0x040009A9 RID: 2473
	public readonly int num;

	// Token: 0x040009AA RID: 2474
	public readonly int den;

	// Token: 0x040009AB RID: 2475
	public static Fraction zero = new Fraction(0, 1);
}
