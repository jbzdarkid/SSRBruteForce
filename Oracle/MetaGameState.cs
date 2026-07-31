using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEngine;

// Token: 0x020000A7 RID: 167
public class MetaGameState
{
	// Token: 0x06000A9C RID: 2716 RVA: 0x00031CDC File Offset: 0x000300DC
	public MetaGameState(string _name, List<string> _islandnames, StringDictionary<string> islandstrings, StringDictionary<Coord> _offsets, StringDictionary<List<string>> _templedat)
	{
		this.name = _name;
		this.islandnames = _islandnames;
		this.islands = new StringDictionary<GameState>();
		List<string> list = new List<string>(this.islandnames);
		foreach (string text in list)
		{
			string text2 = islandstrings[text];
			MetaGameState metaGameState = MetaGameState.Load(text2, true);
			GameState gameState = metaGameState.gamestate;
			bool flag = false;
			foreach (GameState gameState2 in metaGameState.islands.Values)
			{
				gameState2.musicseed = metaGameState.gamestate.musicseed;
				if (gameState2.player != null)
				{
					flag = true;
					break;
				}
			}
			if (flag && gameState.player != null)
			{
				Debug.Log("stripping unnecessary player entrance for level " + text);
				gameState.RemoveEntity(gameState.player);
				List<Entity> list2 = new List<Entity>(gameState.dynamicentities);
				foreach (Entity entity in list2)
				{
					if (entity.type == EntType.sausage)
					{
						gameState.RemoveEntity(entity);
					}
				}
			}
			this.islands.Add(text, gameState);
			foreach (string text3 in metaGameState.islands.Keys)
			{
				string text4 = text + "__" + text3;
				GameState gameState3 = metaGameState.islands[text3];
				this.islands.Add(text4, gameState3);
				this.islandnames.Add(text4);
				islandstrings.Add(text4, gameState3.Save(false, false));
				_offsets.Add(text4, _offsets[text] + metaGameState.gamestate.islandindex[text3].pos);
			}
		}
		this.offsets = _offsets;
		this.templedat = _templedat;
		this.CalcSausagePositions();
		this.CalcPlayerPositions();
		string text5 = string.Empty;
		this.islandmasks = new StringDictionary<IslandMask>();
		foreach (string text6 in this.islandnames)
		{
			Coord coord = this.offsets[text6];
			string text7 = islandstrings[text6];
			string text8 = GameState.Translate(text7, coord);
			text5 = GameState.Merge(text5, text8);
			GameState gameState4 = GameState.Load(text8, null, true);
			IslandMask islandMask = gameState4.LevelMask(coord);
			this.islandmasks.Add(text6, islandMask);
		}
		text5 = this.StripMergedLevel(text5, false);
		this.gamestate = GameState.Load(text5, this, true);
		if (this.islands.ContainsKey("start"))
		{
			this.startpos = this.islands["start"].player.pos + this.offsets["start"];
		}
		this.CalcIslandProjectionCompatibilities();
		this.GenerateCoastDat();
		this.GenerateSplashDat();
		this.GenerateBBQDat();
		this.GenerateTreeDat();
	}

	// Token: 0x06000A9D RID: 2717 RVA: 0x0003216C File Offset: 0x0003056C
	private MetaGameState(string _name)
	{
		this.name = _name;
	}

	// Token: 0x06000A9E RID: 2718 RVA: 0x00032214 File Offset: 0x00030614
	public MetaGameState()
	{
	}

	// Token: 0x06000A9F RID: 2719 RVA: 0x000322B4 File Offset: 0x000306B4
	public static MetaGameState Promote(string name, GameState gamestate)
	{
		return new MetaGameState
		{
			name = name,
			gamestate = gamestate
		};
	}

	// Token: 0x06000AA0 RID: 2720 RVA: 0x000322D8 File Offset: 0x000306D8
	public void RegenCollisionStuff(bool preservestaticents = false)
	{
		for (int i = this.gamestate.dynamicentities.Count - 1; i >= 0; i--)
		{
			Entity entity = this.gamestate.dynamicentities[i];
			if (entity.type == EntType.island)
			{
				this.gamestate.RemoveEntity(entity);
			}
		}
		this.islandmasks = new StringDictionary<IslandMask>();
		string text = string.Empty;
		foreach (string text2 in this.islandnames)
		{
			Coord coord = this.offsets[text2];
			string text3 = this.islands[text2].Save(false, false);
			string text4 = GameState.Translate(text3, coord);
			text = GameState.Merge(text, text4);
			GameState gameState = GameState.Load(text4, null, true);
			IslandMask islandMask = gameState.LevelMask(coord);
			this.islandmasks.Add(text2, islandMask);
		}
		if (!preservestaticents)
		{
			text = this.StripMergedLevel(text, preservestaticents);
			this.gamestate = GameState.Load(text, this, true);
		}
		this.CalcIslandProjectionCompatibilities();
		this.CalcSausagePositions();
		this.CalcPlayerPositions();
		this.GenerateCoastDat();
		this.GenerateSplashDat();
		this.GenerateBBQDat();
		this.GenerateTreeDat();
	}

	// Token: 0x06000AA1 RID: 2721 RVA: 0x00032430 File Offset: 0x00030830
	public void PrintIslandSeeds()
	{
		string text = "Island Seeds\n";
		foreach (KeyValuePair<string, GameState> keyValuePair in this.islands)
		{
			string text2 = text;
			text = string.Concat(new object[]
			{
				text2,
				keyValuePair.Key,
				", ",
				keyValuePair.Value.musicseed,
				"\n"
			});
		}
		Debug.Log(text);
	}

	// Token: 0x06000AA2 RID: 2722 RVA: 0x000324D4 File Offset: 0x000308D4
	public void LoadSmall(string dat)
	{
		this.gamestate = GameState.Load(dat, this, true);
	}

	// Token: 0x06000AA3 RID: 2723 RVA: 0x000324E4 File Offset: 0x000308E4
	private void CalcSausagePositions()
	{
		this.sausagepositions = new StringDictionary<List<KeyValuePair<Coord, Direction>>>();
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			if (gameState == null)
			{
				Debug.Log(text);
			}
			else
			{
				List<KeyValuePair<Coord, Direction>> list = new List<KeyValuePair<Coord, Direction>>();
				IEnumerable<Entity> enumerable = gameState.dynamicentities.Where<Entity>((Entity e) => e.type == EntType.sausage);
				foreach (Entity entity in enumerable)
				{
					list.Add(new KeyValuePair<Coord, Direction>(entity.pos, entity.direction));
				}
				this.sausagepositions.Add(text, list);
			}
		}
	}

	// Token: 0x06000AA4 RID: 2724 RVA: 0x000325FC File Offset: 0x000309FC
	private void CalcPlayerPositions()
	{
		List<string> list = this.islandnames;
		this.playerpositions = new StringDictionary<KeyValuePair<Coord, Direction>>();
		foreach (string text in list)
		{
			if (!this.IsShrine(text) && !(text == "start"))
			{
				GameState gameState = this.islands[text];
				if (gameState.player != null)
				{
					if (gameState.dynamicentities.Any<Entity>((Entity e) => e.type == EntType.sausage))
					{
						this.playerpositions.Add(text, new KeyValuePair<Coord, Direction>(gameState.player.pos, gameState.player.direction));
					}
				}
			}
		}
	}

	// Token: 0x06000AA5 RID: 2725 RVA: 0x000326EC File Offset: 0x00030AEC
	public static MetaGameState Load(string dat, bool precalc = true)
	{
		MetaGameState metaGameState = new MetaGameState();
		metaGameState.Load2(dat, precalc);
		return metaGameState;
	}

	// Token: 0x06000AA6 RID: 2726 RVA: 0x00032708 File Offset: 0x00030B08
	private void RemoveEmptyIslands()
	{
		bool flag = true;
		while (flag)
		{
			flag = false;
			foreach (Entity entity in this.gamestate.dynamicentities)
			{
				if (entity.type == EntType.island)
				{
					if (!this.islands.ContainsKey(entity.dat))
					{
						Debug.Log("removed blank island");
						this.gamestate.RemoveEntity(entity);
						flag = true;
						break;
					}
				}
			}
		}
	}

	// Token: 0x06000AA7 RID: 2727 RVA: 0x000327B8 File Offset: 0x00030BB8
	public void SaveBinary(BinaryWriter dat)
	{
		this.name = "merged";
		dat.Write(this.islandnames.Count);
		foreach (string text in this.islandnames)
		{
			dat.Write(text);
			Coord coord = this.offsets[text];
			dat.Write(coord.x);
			dat.Write(coord.y);
			dat.Write(coord.z);
		}
		dat.Write(this.sausagepositions.Keys.Count);
		foreach (string text2 in this.sausagepositions.Keys)
		{
			dat.Write(text2);
			List<KeyValuePair<Coord, Direction>> list = this.sausagepositions[text2];
			dat.Write(list.Count);
			for (int i = 0; i < list.Count; i++)
			{
				KeyValuePair<Coord, Direction> keyValuePair = list[i];
				Coord key = keyValuePair.Key;
				Direction value = keyValuePair.Value;
				dat.Write(key.x);
				dat.Write(key.y);
				dat.Write(key.z);
				dat.Write((int)value);
			}
		}
		dat.Write(this.playerpositions.Keys.Count<string>());
		foreach (string text3 in this.playerpositions.Keys)
		{
			dat.Write(text3);
			KeyValuePair<Coord, Direction> keyValuePair2 = this.playerpositions[text3];
			Coord key2 = keyValuePair2.Key;
			Direction value2 = keyValuePair2.Value;
			dat.Write(key2.x);
			dat.Write(key2.y);
			dat.Write(key2.z);
			dat.Write((int)value2);
		}
		dat.Write(this.templedat.Keys.Count);
		foreach (string text4 in this.templedat.Keys)
		{
			dat.Write(text4);
			List<string> list2 = this.templedat[text4];
			dat.Write(list2.Count);
			foreach (string text5 in list2)
			{
				dat.Write(text5);
			}
		}
		dat.Write(this.islandmasks.Keys.Count);
		foreach (string text6 in this.islandmasks.Keys)
		{
			dat.Write(text6);
			IslandMask islandMask = this.islandmasks[text6];
			int num = islandMask.mask.Length;
			int num2 = ((num <= 0) ? 0 : islandMask.mask[0].Length);
			int num3 = ((num2 <= 0) ? 0 : islandMask.mask[0][0].Length);
			dat.Write(num);
			dat.Write(num2);
			dat.Write(num3);
			for (int j = 0; j < num; j++)
			{
				for (int k = 0; k < num2; k++)
				{
					for (int l = 0; l < num3; l++)
					{
						dat.Write(islandMask.mask[j][k][l]);
					}
				}
			}
			dat.Write(islandMask.offset.x);
			dat.Write(islandMask.offset.y);
			dat.Write(islandMask.offset.z);
		}
		dat.Write(this.projectioncompatibilities.Keys.Count);
		foreach (string text7 in this.projectioncompatibilities.Keys)
		{
			dat.Write(text7);
			StringDictionary<bool[,]> stringDictionary = this.projectioncompatibilities[text7];
			dat.Write(stringDictionary.Keys.Count);
			foreach (string text8 in stringDictionary.Keys)
			{
				dat.Write(text8);
				bool[,] array = stringDictionary[text8];
				dat.Write(array.GetLength(0));
				dat.Write(array.GetLength(1));
				for (int m = 0; m < array.GetLength(0); m++)
				{
					for (int n = 0; n < array.GetLength(1); n++)
					{
						dat.Write(array[m, n]);
					}
				}
			}
		}
		dat.Write(this.coastdat.Keys.Count);
		foreach (string text9 in this.coastdat.Keys)
		{
			dat.Write(text9);
			IntDictionary<List<Coord>> intDictionary = this.coastdat[text9];
			dat.Write(intDictionary.Keys.Count);
			foreach (int num4 in intDictionary.Keys)
			{
				dat.Write(num4);
				List<Coord> list3 = intDictionary[num4];
				dat.Write(list3.Count);
				foreach (Coord coord2 in list3)
				{
					dat.Write(coord2.x);
					dat.Write(coord2.y);
					dat.Write(coord2.z);
				}
			}
		}
		dat.Write(this.splashdat.Keys.Count);
		foreach (string text10 in this.splashdat.Keys)
		{
			dat.Write(text10);
			IntDictionary<List<Coord>> intDictionary2 = this.splashdat[text10];
			dat.Write(intDictionary2.Keys.Count);
			foreach (int num5 in intDictionary2.Keys)
			{
				dat.Write(num5);
				List<Coord> list4 = intDictionary2[num5];
				dat.Write(list4.Count);
				foreach (Coord coord3 in list4)
				{
					dat.Write(coord3.x);
					dat.Write(coord3.y);
					dat.Write(coord3.z);
				}
			}
		}
		dat.Write(this.bbqdat.Keys.Count);
		foreach (string text11 in this.bbqdat.Keys)
		{
			dat.Write(text11);
			List<Coord> list5 = this.bbqdat[text11];
			dat.Write(list5.Count);
			foreach (Coord coord4 in list5)
			{
				dat.Write(coord4.x);
				dat.Write(coord4.y);
				dat.Write(coord4.z);
			}
		}
		dat.Write(this.treedat.Keys.Count);
		foreach (string text12 in this.treedat.Keys)
		{
			dat.Write(text12);
			List<KeyValuePair<Coord, int>> list6 = this.treedat[text12];
			dat.Write(list6.Count);
			foreach (KeyValuePair<Coord, int> keyValuePair3 in list6)
			{
				dat.Write(keyValuePair3.Key.x);
				dat.Write(keyValuePair3.Key.y);
				dat.Write(keyValuePair3.Key.z);
				dat.Write(keyValuePair3.Value);
			}
		}
		dat.Write(this.gamestate.Save(false, false));
		dat.Write(this.islands.Keys.Count);
		foreach (string text13 in this.islands.Keys)
		{
			dat.Write(text13);
			GameState gameState = this.islands[text13];
			dat.Write(gameState.Save(false, false));
		}
	}

	// Token: 0x06000AA8 RID: 2728 RVA: 0x00033428 File Offset: 0x00031828
	public void LoadBinary(BinaryReader dat)
	{
		this.name = "merged";
		int num = dat.ReadInt32();
		this.islandnames = new List<string>(num);
		this.offsets = new StringDictionary<Coord>(num);
		for (int i = 0; i < num; i++)
		{
			string text = dat.ReadString();
			Coord coord = new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32());
			this.offsets.Add(text, coord);
			this.islandnames.Add(text);
		}
		int num2 = dat.ReadInt32();
		this.sausagepositions = new StringDictionary<List<KeyValuePair<Coord, Direction>>>(num2);
		for (int j = 0; j < num2; j++)
		{
			string text2 = dat.ReadString();
			int num3 = dat.ReadInt32();
			List<KeyValuePair<Coord, Direction>> list = new List<KeyValuePair<Coord, Direction>>(num3);
			for (int k = 0; k < num3; k++)
			{
				Coord coord2 = new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32());
				Direction direction = (Direction)dat.ReadInt32();
				KeyValuePair<Coord, Direction> keyValuePair = new KeyValuePair<Coord, Direction>(coord2, direction);
				list.Add(keyValuePair);
			}
			this.sausagepositions.Add(text2, list);
		}
		int num4 = dat.ReadInt32();
		this.playerpositions = new StringDictionary<KeyValuePair<Coord, Direction>>(num4);
		for (int l = 0; l < num4; l++)
		{
			string text3 = dat.ReadString();
			Coord coord3 = new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32());
			Direction direction2 = (Direction)dat.ReadInt32();
			KeyValuePair<Coord, Direction> keyValuePair2 = new KeyValuePair<Coord, Direction>(coord3, direction2);
			this.playerpositions.Add(text3, keyValuePair2);
		}
		int num5 = dat.ReadInt32();
		this.templedat = new StringDictionary<List<string>>(num5);
		for (int m = 0; m < num5; m++)
		{
			string text4 = dat.ReadString();
			int num6 = dat.ReadInt32();
			List<string> list2 = new List<string>(num6);
			for (int n = 0; n < num6; n++)
			{
				list2.Add(dat.ReadString());
			}
			this.templedat.Add(text4, list2);
		}
		int num7 = dat.ReadInt32();
		this.islandmasks = new StringDictionary<IslandMask>(num7);
		for (int num8 = 0; num8 < num7; num8++)
		{
			string text5 = dat.ReadString();
			int num9 = dat.ReadInt32();
			int num10 = dat.ReadInt32();
			int num11 = dat.ReadInt32();
			int[][][] array = new int[num9][][];
			for (int num12 = 0; num12 < num9; num12++)
			{
				array[num12] = new int[num10][];
				for (int num13 = 0; num13 < num10; num13++)
				{
					array[num12][num13] = new int[num11];
					for (int num14 = 0; num14 < num11; num14++)
					{
						array[num12][num13][num14] = dat.ReadInt32();
					}
				}
			}
			Coord coord4 = new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32());
			IslandMask islandMask = new IslandMask(array, coord4);
			this.islandmasks.Add(text5, islandMask);
		}
		int num15 = dat.ReadInt32();
		this.projectioncompatibilities = new StringDictionary<StringDictionary<bool[,]>>(num15);
		for (int num16 = 0; num16 < num15; num16++)
		{
			string text6 = dat.ReadString();
			int num17 = dat.ReadInt32();
			StringDictionary<bool[,]> stringDictionary = new StringDictionary<bool[,]>(num17);
			for (int num18 = 0; num18 < num17; num18++)
			{
				string text7 = dat.ReadString();
				int num19 = dat.ReadInt32();
				int num20 = dat.ReadInt32();
				bool[,] array2 = new bool[num19, num20];
				byte[] array3 = dat.ReadBytes(num19 * num20);
				for (int num21 = 0; num21 < num19; num21++)
				{
					for (int num22 = 0; num22 < num20; num22++)
					{
						array2[num21, num22] = array3[num21 * num20 + num22] != 0;
					}
				}
				stringDictionary.Add(text7, array2);
			}
			this.projectioncompatibilities.Add(text6, stringDictionary);
		}
		int num23 = dat.ReadInt32();
		this.coastdat = new StringDictionary<IntDictionary<List<Coord>>>(num23);
		for (int num24 = 0; num24 < num23; num24++)
		{
			string text8 = dat.ReadString();
			int num25 = dat.ReadInt32();
			IntDictionary<List<Coord>> intDictionary = new IntDictionary<List<Coord>>(num25);
			for (int num26 = 0; num26 < num25; num26++)
			{
				int num27 = dat.ReadInt32();
				int num28 = dat.ReadInt32();
				List<Coord> list3 = new List<Coord>(num28);
				for (int num29 = 0; num29 < num28; num29++)
				{
					list3.Add(new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32()));
				}
				intDictionary.Add(num27, list3);
			}
			this.coastdat.Add(text8, intDictionary);
		}
		int num30 = dat.ReadInt32();
		this.splashdat = new StringDictionary<IntDictionary<List<Coord>>>(num30);
		for (int num31 = 0; num31 < num30; num31++)
		{
			string text9 = dat.ReadString();
			int num32 = dat.ReadInt32();
			IntDictionary<List<Coord>> intDictionary2 = new IntDictionary<List<Coord>>(num32);
			for (int num33 = 0; num33 < num32; num33++)
			{
				int num34 = dat.ReadInt32();
				int num35 = dat.ReadInt32();
				List<Coord> list4 = new List<Coord>(num35);
				for (int num36 = 0; num36 < num35; num36++)
				{
					list4.Add(new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32()));
				}
				intDictionary2.Add(num34, list4);
			}
			this.splashdat.Add(text9, intDictionary2);
		}
		int num37 = dat.ReadInt32();
		this.bbqdat = new StringDictionary<List<Coord>>(num37);
		for (int num38 = 0; num38 < num37; num38++)
		{
			string text10 = dat.ReadString();
			int num39 = dat.ReadInt32();
			List<Coord> list5 = new List<Coord>(num39);
			for (int num40 = 0; num40 < num39; num40++)
			{
				list5.Add(new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32()));
			}
			this.bbqdat.Add(text10, list5);
		}
		int num41 = dat.ReadInt32();
		this.treedat = new StringDictionary<List<KeyValuePair<Coord, int>>>(num41);
		for (int num42 = 0; num42 < num41; num42++)
		{
			string text11 = dat.ReadString();
			int num43 = dat.ReadInt32();
			List<KeyValuePair<Coord, int>> list6 = new List<KeyValuePair<Coord, int>>(num43);
			for (int num44 = 0; num44 < num43; num44++)
			{
				Coord coord5 = new Coord(dat.ReadInt32(), dat.ReadInt32(), dat.ReadInt32());
				int num45 = dat.ReadInt32();
				KeyValuePair<Coord, int> keyValuePair3 = new KeyValuePair<Coord, int>(coord5, num45);
				list6.Add(keyValuePair3);
			}
			this.treedat.Add(text11, list6);
		}
		string text12 = dat.ReadString();
		this.gamestate = GameState.Load(text12, this, true);
		int num46 = dat.ReadInt32();
		this.islands = new StringDictionary<GameState>(num46);
		for (int num47 = 0; num47 < num46; num47++)
		{
			string text13 = dat.ReadString();
			string text14 = dat.ReadString();
			this.islands.Add(text13, GameState.Load(text14, null, false));
		}
		if (this.islands.ContainsKey("start"))
		{
			this.startpos = this.islands["start"].player.pos + this.offsets["start"];
		}
	}

	// Token: 0x06000AA9 RID: 2729 RVA: 0x00033B80 File Offset: 0x00031F80
	public void Load2(string dat, bool precalc = true)
	{
		if (dat[0] != 'V')
		{
			this.LoadSmall(dat);
			return;
		}
		string[] array = dat.Split(new char[] { '\n' });
		this.name = array[1];
		string[] array2 = array[3].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
		this.islandnames = array2.ToList<string>();
		string[] array3 = array[5].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
		this.offsets = new StringDictionary<Coord>();
		int num = 0;
		while (num + 3 < array3.Length)
		{
			string text = array3[num];
			Coord coord = new Coord(array3[num + 1].IntParseFast(), array3[num + 2].IntParseFast(), array3[num + 3].IntParseFast());
			this.offsets.Add(text, coord);
			num += 4;
		}
		this.sausagepositions = new StringDictionary<List<KeyValuePair<Coord, Direction>>>();
		string[] array4 = array[6].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text2 in array4)
		{
			string[] array6 = text2.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text3 = array6[0];
			List<KeyValuePair<Coord, Direction>> list = new List<KeyValuePair<Coord, Direction>>();
			int num2 = 1;
			while (num2 + 3 < array6.Length)
			{
				Coord coord2 = new Coord(array6[num2].IntParseFast(), array6[num2 + 1].IntParseFast(), array6[num2 + 2].IntParseFast());
				Direction direction = (Direction)array6[num2 + 3].IntParseFast();
				list.Add(new KeyValuePair<Coord, Direction>(coord2, direction));
				num2 += 4;
			}
			this.sausagepositions.Add(text3, list);
		}
		this.playerpositions = new StringDictionary<KeyValuePair<Coord, Direction>>();
		string[] array7 = array[7].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text4 in array7)
		{
			string[] array9 = text4.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text5 = array9[0];
			Coord coord3 = new Coord(array9[1].IntParseFast(), array9[2].IntParseFast(), array9[3].IntParseFast());
			Direction direction2 = (Direction)array9[4].IntParseFast();
			this.playerpositions.Add(text5, new KeyValuePair<Coord, Direction>(coord3, direction2));
		}
		this.templedat = new StringDictionary<List<string>>();
		string[] array10 = array[8].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text6 in array10)
		{
			string[] array12 = text6.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text7 = array12[0];
			List<string> list2 = new List<string>();
			for (int l = 1; l < array12.Length; l++)
			{
				list2.Add(array12[l]);
			}
			this.templedat.Add(text7, list2);
		}
		this.islandmasks = new StringDictionary<IslandMask>();
		string[] array13 = array[9].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text8 in array13)
		{
			string[] array15 = text8.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text9 = array15[0];
			Coord coord4 = new Coord(array15[1].IntParseFast(), array15[2].IntParseFast(), array15[3].IntParseFast());
			coord4 += this.offsets[text9];
			int num3 = array15[4].IntParseFast();
			int num4 = array15[5].IntParseFast();
			int num5 = array15[6].IntParseFast();
			int[][][] array16 = new int[num3][][];
			for (int n = 0; n < num3; n++)
			{
				array16[n] = new int[num4][];
				for (int num6 = 0; num6 < num4; num6++)
				{
					array16[n][num6] = new int[num5];
					for (int num7 = 0; num7 < num5; num7++)
					{
						array16[n][num6][num7] = array15[7 + num7 + num5 * num6 + num5 * num4 * n].IntParseFast();
					}
				}
			}
			IslandMask islandMask = new IslandMask(array16, coord4);
			this.islandmasks.Add(text9, islandMask);
		}
		this.projectioncompatibilities = new StringDictionary<StringDictionary<bool[,]>>();
		string[] array17 = array[10].Split(new char[] { '$' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text10 in array17)
		{
			string[] array19 = text10.Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
			string text11 = array19[0];
			StringDictionary<bool[,]> stringDictionary = new StringDictionary<bool[,]>();
			for (int num9 = 1; num9 < array19.Length; num9++)
			{
				string[] array20 = array19[num9].Split(new char[] { ',' });
				string text12 = array20[0];
				int num10 = array20[1].IntParseFast();
				int num11 = array20[2].IntParseFast();
				bool[,] array21 = new bool[num10, num11];
				string text13 = array20[3];
				for (int num12 = 0; num12 < num10; num12++)
				{
					for (int num13 = 0; num13 < num11; num13++)
					{
						if (text13[num13 + num11 * num12] == '1')
						{
							array21[num12, num13] = true;
						}
					}
				}
				stringDictionary.Add(text12, array21);
			}
			this.projectioncompatibilities.Add(text11, stringDictionary);
		}
		this.coastdat = new StringDictionary<IntDictionary<List<Coord>>>();
		string[] array22 = array[11].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text14 in array22)
		{
			string[] array24 = text14.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text15 = array24[0];
			IntDictionary<List<Coord>> intDictionary = new IntDictionary<List<Coord>>();
			int num15 = 1;
			while (num15 + 3 < array24.Length)
			{
				int num16 = array24[num15].IntParseFast();
				Coord coord5 = new Coord(array24[num15 + 1].IntParseFast(), array24[num15 + 2].IntParseFast(), array24[num15 + 3].IntParseFast());
				if (!intDictionary.ContainsKey(num16))
				{
					intDictionary.Add(num16, new List<Coord>());
				}
				intDictionary[num16].Add(coord5);
				num15 += 4;
			}
			this.coastdat.Add(text15, intDictionary);
		}
		this.splashdat = new StringDictionary<IntDictionary<List<Coord>>>();
		string[] array25 = array[12].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text16 in array25)
		{
			string[] array27 = text16.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text17 = array27[0];
			IntDictionary<List<Coord>> intDictionary2 = new IntDictionary<List<Coord>>();
			int num18 = 1;
			while (num18 + 3 < array27.Length)
			{
				int num19 = array27[num18].IntParseFast();
				Coord coord6 = new Coord(array27[num18 + 1].IntParseFast(), array27[num18 + 2].IntParseFast(), array27[num18 + 3].IntParseFast());
				if (!intDictionary2.ContainsKey(num19))
				{
					intDictionary2.Add(num19, new List<Coord>());
				}
				intDictionary2[num19].Add(coord6);
				num18 += 4;
			}
			this.splashdat.Add(text17, intDictionary2);
		}
		this.bbqdat = new StringDictionary<List<Coord>>();
		string[] array28 = array[13].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text18 in array28)
		{
			string[] array30 = text18.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text19 = array30[0];
			List<Coord> list3 = new List<Coord>();
			int num21 = 1;
			while (num21 + 2 < array30.Length)
			{
				Coord coord7 = new Coord(array30[num21].IntParseFast(), array30[num21 + 1].IntParseFast(), array30[num21 + 2].IntParseFast());
				list3.Add(coord7);
				num21 += 3;
			}
			this.bbqdat.Add(text19, list3);
		}
		this.treedat = new StringDictionary<List<KeyValuePair<Coord, int>>>();
		string[] array31 = array[14].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text20 in array31)
		{
			string[] array33 = text20.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			string text21 = array33[0];
			List<KeyValuePair<Coord, int>> list4 = new List<KeyValuePair<Coord, int>>();
			int num23 = 1;
			while (num23 + 3 < array33.Length)
			{
				Coord coord8 = new Coord(array33[num23].IntParseFast(), array33[num23 + 1].IntParseFast(), array33[num23 + 2].IntParseFast());
				int num24 = array33[num23 + 3].IntParseFast();
				list4.Add(new KeyValuePair<Coord, int>(coord8, num24));
				num23 += 4;
			}
			this.treedat.Add(text21, list4);
		}
		this.gamestate = GameState.Load(array[2], this, precalc);
		string[] array34 = array[4].Split(new char[] { '@' }, StringSplitOptions.RemoveEmptyEntries);
		this.islands = new StringDictionary<GameState>();
		int num25 = 0;
		while (num25 + 1 < array34.Length)
		{
			string text22 = array34[num25];
			string text23 = array34[num25 + 1];
			this.islands.Add(text22, GameState.Load(text23, null, false));
			num25 += 2;
		}
		if (this.islands.ContainsKey("start"))
		{
			this.startpos = this.islands["start"].player.pos + this.offsets["start"];
		}
	}

	// Token: 0x06000AAA RID: 2730 RVA: 0x0003453C File Offset: 0x0003293C
	public static MetaGameState LoadFromPath(string path, bool precalc = true, bool binary = false)
	{
		if (binary)
		{
			path = "Generated/merged_binary";
			TextAsset textAsset = (TextAsset)Resources.Load(path);
			using (BinaryReader binaryReader = new BinaryReader(new MemoryStream(textAsset.bytes)))
			{
				MetaGameState metaGameState = new MetaGameState();
				metaGameState.LoadBinary(binaryReader);
				return metaGameState;
			}
		}
		return null;
	}

	// Token: 0x06000AAB RID: 2731 RVA: 0x000345A8 File Offset: 0x000329A8
	public void NormalizeState()
	{
		List<string> list = new List<string>();
		foreach (KeyValuePair<string, GameState> keyValuePair in this.islands)
		{
			keyValuePair.Value.NormalizeState(false);
			if (keyValuePair.Value.entities.Count == 0)
			{
				list.Add(keyValuePair.Key);
			}
		}
		foreach (string text in list)
		{
			this.islandnames.Remove(text);
			this.islands.Remove(text);
			this.coastdat.Remove(text);
			this.splashdat.Remove(text);
			this.bbqdat.Remove(text);
			this.treedat.Remove(text);
			this.offsets.Remove(text);
			this.islandmasks.Remove(text);
			this.playerpositions.Remove(text);
			this.sausagepositions.Remove(text);
			foreach (Entity entity in this.gamestate.dynamicentities)
			{
				if (entity.type == EntType.island && entity.dat == text)
				{
					this.gamestate.RemoveEntity(entity);
					break;
				}
			}
		}
		this.gamestate.NormalizeState(false);
		this.RegenIslands();
		foreach (Entity entity2 in this.gamestate.entities)
		{
			entity2.PrecalcAll();
		}
	}

	// Token: 0x06000AAC RID: 2732 RVA: 0x00034808 File Offset: 0x00032C08
	public string Save(bool islandinheritmusicseed)
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append('V');
		stringBuilder.Append(this.version);
		stringBuilder.Append('\n');
		stringBuilder.Append(this.name);
		stringBuilder.Append('\n');
		if (this.gamestate.displayname == string.Empty)
		{
			this.gamestate.displayname = this.name;
		}
		foreach (string text in this.islands.Keys)
		{
			GameState gameState = this.islands[text];
			if (gameState.displayname == string.Empty)
			{
				gameState.displayname = text;
			}
			if (islandinheritmusicseed)
			{
				gameState.musicseed = this.gamestate.musicseed;
			}
			else if (gameState.musicseed == 0)
			{
				gameState.musicseed = text.GetHashCode();
			}
		}
		string text2;
		if (this.gamestate != null)
		{
			text2 = this.gamestate.Save(false, false);
		}
		else
		{
			text2 = string.Empty;
		}
		stringBuilder.Append(text2);
		stringBuilder.Append('\n');
		for (int i = 0; i < this.islandnames.Count; i++)
		{
			stringBuilder.Append(this.islandnames[i]);
			stringBuilder.Append(',');
		}
		stringBuilder.Append('\n');
		foreach (string text3 in this.islands.Keys)
		{
			stringBuilder.Append(text3);
			stringBuilder.Append('@');
			GameState gameState2 = this.islands[text3];
			string text4;
			if (this.gamestate != null)
			{
				text4 = gameState2.Save(false, false);
			}
			else
			{
				text4 = string.Empty;
			}
			stringBuilder.Append(text4);
			stringBuilder.Append('@');
		}
		stringBuilder.Append('\n');
		foreach (string text5 in this.offsets.Keys)
		{
			Coord coord = this.offsets[text5];
			stringBuilder.Append(text5);
			stringBuilder.Append(',');
			stringBuilder.Append(coord.x.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(coord.y.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(coord.z.ToStringFast());
			stringBuilder.Append(',');
		}
		stringBuilder.Append('\n');
		foreach (string text6 in this.sausagepositions.Keys)
		{
			stringBuilder.Append(text6);
			stringBuilder.Append(',');
			List<KeyValuePair<Coord, Direction>> list = this.sausagepositions[text6];
			foreach (KeyValuePair<Coord, Direction> keyValuePair in list)
			{
				stringBuilder.Append(keyValuePair.Key.x.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(keyValuePair.Key.y.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(keyValuePair.Key.z.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(((int)keyValuePair.Value).ToStringFast());
				stringBuilder.Append(',');
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text7 in this.playerpositions.Keys)
		{
			stringBuilder.Append(text7);
			stringBuilder.Append(',');
			KeyValuePair<Coord, Direction> keyValuePair2 = this.playerpositions[text7];
			stringBuilder.Append(keyValuePair2.Key.x.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(keyValuePair2.Key.y.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(keyValuePair2.Key.z.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(((int)keyValuePair2.Value).ToStringFast());
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text8 in this.templedat.Keys)
		{
			stringBuilder.Append(text8);
			stringBuilder.Append(',');
			List<string> list2 = this.templedat[text8];
			foreach (string text9 in list2)
			{
				stringBuilder.Append(text9);
				stringBuilder.Append(',');
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text10 in this.islandmasks.Keys)
		{
			IslandMask islandMask = this.islandmasks[text10];
			stringBuilder.Append(text10);
			stringBuilder.Append(',');
			Coord coord2 = islandMask.offset - this.offsets[text10];
			stringBuilder.Append(coord2.x.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(coord2.y.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(coord2.z.ToStringFast());
			stringBuilder.Append(',');
			int num = islandMask.mask.Length;
			int num2 = ((num != 0) ? islandMask.mask[0].Length : 0);
			int num3 = ((num2 != 0) ? islandMask.mask[0][0].Length : 0);
			stringBuilder.Append(num.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(num2.ToStringFast());
			stringBuilder.Append(',');
			stringBuilder.Append(num3.ToStringFast());
			stringBuilder.Append(',');
			for (int j = 0; j < num; j++)
			{
				for (int k = 0; k < num2; k++)
				{
					for (int l = 0; l < num3; l++)
					{
						stringBuilder.Append(islandMask.mask[j][k][l].ToStringFast());
						stringBuilder.Append(',');
					}
				}
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text11 in this.projectioncompatibilities.Keys)
		{
			stringBuilder.Append(text11);
			stringBuilder.Append('|');
			StringDictionary<bool[,]> stringDictionary = this.projectioncompatibilities[text11];
			foreach (string text12 in stringDictionary.Keys)
			{
				stringBuilder.Append(text12);
				stringBuilder.Append(',');
				bool[,] array = stringDictionary[text12];
				int length = array.GetLength(0);
				int length2 = array.GetLength(1);
				stringBuilder.Append(length.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(length2.ToStringFast());
				stringBuilder.Append(',');
				for (int m = 0; m < length; m++)
				{
					for (int n = 0; n < length2; n++)
					{
						stringBuilder.Append((!array[m, n]) ? '0' : '1');
					}
				}
				stringBuilder.Append('|');
			}
			stringBuilder.Append('$');
		}
		stringBuilder.Append('\n');
		foreach (string text13 in this.coastdat.Keys)
		{
			stringBuilder.Append(text13);
			stringBuilder.Append(',');
			IntDictionary<List<Coord>> intDictionary = this.coastdat[text13];
			foreach (int num4 in intDictionary.Keys)
			{
				foreach (Coord coord3 in intDictionary[num4])
				{
					stringBuilder.Append(num4.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord3.x.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord3.y.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord3.z.ToStringFast());
					stringBuilder.Append(',');
				}
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text14 in this.splashdat.Keys)
		{
			stringBuilder.Append(text14);
			stringBuilder.Append(',');
			IntDictionary<List<Coord>> intDictionary2 = this.splashdat[text14];
			foreach (int num5 in intDictionary2.Keys)
			{
				foreach (Coord coord4 in intDictionary2[num5])
				{
					stringBuilder.Append(num5.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord4.x.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord4.y.ToStringFast());
					stringBuilder.Append(',');
					stringBuilder.Append(coord4.z.ToStringFast());
					stringBuilder.Append(',');
				}
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text15 in this.bbqdat.Keys)
		{
			stringBuilder.Append(text15);
			stringBuilder.Append(',');
			List<Coord> list3 = this.bbqdat[text15];
			foreach (Coord coord5 in list3)
			{
				stringBuilder.Append(coord5.x.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(coord5.y.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(coord5.z.ToStringFast());
				stringBuilder.Append(',');
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		foreach (string text16 in this.treedat.Keys)
		{
			stringBuilder.Append(text16);
			stringBuilder.Append(',');
			List<KeyValuePair<Coord, int>> list4 = this.treedat[text16];
			foreach (KeyValuePair<Coord, int> keyValuePair3 in list4)
			{
				stringBuilder.Append(keyValuePair3.Key.x.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(keyValuePair3.Key.y.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(keyValuePair3.Key.z.ToStringFast());
				stringBuilder.Append(',');
				stringBuilder.Append(keyValuePair3.Value.ToStringFast());
				stringBuilder.Append(',');
			}
			stringBuilder.Append('|');
		}
		stringBuilder.Append('\n');
		return stringBuilder.ToString();
	}

	// Token: 0x06000AAD RID: 2733 RVA: 0x00035820 File Offset: 0x00033C20
	private static byte[] GetBytes(string str)
	{
		byte[] array = new byte[str.Length * 2];
		Buffer.BlockCopy(str.ToCharArray(), 0, array, 0, array.Length);
		return array;
	}

	// Token: 0x06000AAE RID: 2734 RVA: 0x00035850 File Offset: 0x00033C50
	private void GenerateCoastDat()
	{
		this.coastdat = new StringDictionary<IntDictionary<List<Coord>>>();
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			IntDictionary<List<Coord>> intDictionary = gameState.CoastData();
			this.coastdat.Add(text, intDictionary);
		}
	}

	// Token: 0x06000AAF RID: 2735 RVA: 0x000358D4 File Offset: 0x00033CD4
	private void GenerateSplashDat()
	{
		this.splashdat = new StringDictionary<IntDictionary<List<Coord>>>();
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			IntDictionary<List<Coord>> intDictionary = gameState.SplashData();
			this.splashdat.Add(text, intDictionary);
		}
	}

	// Token: 0x06000AB0 RID: 2736 RVA: 0x00035958 File Offset: 0x00033D58
	private void GenerateBBQDat()
	{
		this.bbqdat = new StringDictionary<List<Coord>>();
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			List<Coord> list = new List<Coord>();
			gameState.CalcBBQPositions(list);
			this.bbqdat.Add(text, list);
		}
	}

	// Token: 0x06000AB1 RID: 2737 RVA: 0x000359E0 File Offset: 0x00033DE0
	private void GenerateTreeDat()
	{
		this.treedat = new StringDictionary<List<KeyValuePair<Coord, int>>>();
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			List<KeyValuePair<Coord, int>> list = gameState.TreePositions();
			this.treedat.Add(text, list);
		}
	}

	// Token: 0x06000AB2 RID: 2738 RVA: 0x00035A64 File Offset: 0x00033E64
	private string StripMergedLevel(string merged, bool preservenonislands = false)
	{
		GameState gameState = GameState.Load(merged, null, true);
		gameState.entities = gameState.entities.Where<Entity>((Entity e) => (preservenonislands || !e.type.Static()) && e.type != EntType.island && e.type != EntType.barrier).ToList<Entity>();
		gameState.dynamicentities = gameState.entities.Where<Entity>((Entity e) => e.type.Dynamic()).ToList<Entity>();
		if (gameState.entities.Count > 0)
		{
			gameState.SetIDCounterTo(gameState.entities.Max<Entity>((Entity e) => e.id) + 1);
		}
		foreach (string text in this.islandnames)
		{
			gameState.AddEntity(new Entity(gameState)
			{
				type = EntType.island,
				pos = this.offsets[text],
				direction = Direction.None,
				dat = text,
				tileset = this.islands[text].tileset
			}, false);
		}
		string text2;
		if (this.islandnames.Contains("start"))
		{
			text2 = "start";
		}
		else
		{
			text2 = this.islands.FirstOrDefault<KeyValuePair<string, GameState>>((KeyValuePair<string, GameState> x) => x.Value.player != null).Key;
		}
		string text3 = text2;
		Coord coord = ((text3 == null) ? Coord.Zero : (this.islands[text3].player.pos + this.offsets[text3]));
		if (!preservenonislands)
		{
			int i = gameState.entities.Count - 1;
			while (i >= 0)
			{
				bool flag = false;
				Entity entity = gameState.entities[i];
				switch (entity.type)
				{
				case EntType.ground:
					if (entity.Decoration())
					{
						flag = true;
					}
					break;
				case EntType.player:
					flag = true;
					break;
				case EntType.sausage:
					flag = true;
					break;
				}
				IL_0245:
				if (flag)
				{
					gameState.dynamicentities.Remove(entity);
					gameState.entities.RemoveAt(i);
					gameState.dynamicentityindex.Remove(entity.id);
					if (entity.type == EntType.island)
					{
						gameState.islandindex.Remove(entity.dat);
					}
				}
				i--;
				continue;
				goto IL_0245;
			}
		}
		gameState.AddEntity(new Entity(gameState)
		{
			type = EntType.player,
			pos = coord,
			direction = Direction.North
		}, false);
		gameState.UpdateSMap();
		gameState.UpdateDynamicIDDict();
		return gameState.Save(false, true);
	}

	// Token: 0x06000AB3 RID: 2739 RVA: 0x00035D68 File Offset: 0x00034168
	public static MetaGameState OverWorld()
	{
		return new MetaGameState();
	}

	// Token: 0x06000AB4 RID: 2740 RVA: 0x00035D70 File Offset: 0x00034170
	public static MetaGameState Blank()
	{
		return new MetaGameState();
	}

	// Token: 0x06000AB5 RID: 2741 RVA: 0x00035D84 File Offset: 0x00034184
	public void LoadIslandMasksAndComputeProjections(string filename, bool force = false)
	{
		this.islandmasks = new StringDictionary<IslandMask>();
		TextAsset textAsset = (TextAsset)Resources.Load(filename, typeof(TextAsset));
		if (textAsset == null)
		{
			return;
		}
		string[] array = textAsset.text.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
		foreach (string text in array)
		{
			string[] array3 = text.Split(new char[] { '|' });
			IslandMask islandMask = IslandMask.FromString(array3[1]);
			this.islandmasks.Add(array3[0], islandMask);
			this.islandmasks.Add(array3[0], islandMask.Projection());
		}
		this.CalcIslandProjectionCompatibilities();
	}

	// Token: 0x06000AB6 RID: 2742 RVA: 0x00035E44 File Offset: 0x00034244
	private void CalcIslandProjectionCompatibilities()
	{
		this.projectioncompatibilities = new StringDictionary<StringDictionary<bool[,]>>();
		foreach (string text in this.islandmasks.Keys)
		{
			StringDictionary<bool[,]> stringDictionary = new StringDictionary<bool[,]>();
			foreach (string text2 in this.islandmasks.Keys)
			{
				IslandMask islandMask = this.islandmasks[text].Projection();
				IslandMask islandMask2 = this.islandmasks[text2].Projection();
				int num = islandMask.mask.Length;
				int num2 = ((num != 0) ? islandMask.mask[0].Length : 0);
				int num3 = islandMask2.mask.Length;
				int num4 = ((num3 != 0) ? islandMask2.mask[0].Length : 0);
				bool[,] array = new bool[Math.Max(num + num3 - 1, 0), Math.Max(num2 + num4 - 1, 0)];
				for (int i = -(num - 1); i < num3; i++)
				{
					for (int j = -(num2 - 1); j < num4; j++)
					{
						bool flag = false;
						int num5 = Math.Min(num3 - i, num);
						int num6 = Math.Min(num4 - j, num2);
						for (int k = Math.Max(-i, 0); k < num5; k++)
						{
							for (int l = Math.Max(-j, 0); l < num6; l++)
							{
								if (islandMask.mask[k][l][0] > 0 && islandMask2.mask[k + i][l + j][0] > 0)
								{
									flag = true;
									break;
								}
							}
							if (flag)
							{
								break;
							}
						}
						array[i + num - 1, j + num2 - 1] = !flag;
					}
				}
				stringDictionary.Add(text2, array);
			}
			this.projectioncompatibilities.Add(text, stringDictionary);
		}
	}

	// Token: 0x06000AB7 RID: 2743 RVA: 0x000360B4 File Offset: 0x000344B4
	public void RegenIslands()
	{
		foreach (string text in this.islandnames)
		{
			foreach (Entity entity in this.gamestate.dynamicentities)
			{
				if (entity.type == EntType.island && entity.dat == text)
				{
					this.offsets[text] = entity.pos;
					break;
				}
			}
		}
		this.RegenCollisionStuff(true);
		foreach (string text2 in this.islandnames)
		{
			Entity entity2 = new Entity(this.gamestate);
			entity2.type = EntType.island;
			entity2.pos = this.offsets[text2];
			entity2.direction = Direction.None;
			entity2.dat = text2;
			entity2.tileset = this.islands[text2].tileset;
			this.gamestate.AddEntity(entity2, false);
		}
		this.gamestate.UpdateDynamicIDDict();
	}

	// Token: 0x06000AB8 RID: 2744 RVA: 0x00036220 File Offset: 0x00034620
	public bool IsShrine(string shrinename)
	{
		return this.templedat.ContainsKey(shrinename);
	}

	// Token: 0x06000AB9 RID: 2745 RVA: 0x0003622E File Offset: 0x0003462E
	public static void ClearSave()
	{
		SaveGame.DeleteAll();
	}

	// Token: 0x06000ABA RID: 2746 RVA: 0x00036238 File Offset: 0x00034638
	public void StripDecorations()
	{
		int num = 1;
		foreach (GameState gameState in this.islands.Values)
		{
			gameState.StripDecorations(MetaGameState.points[num]);
			gameState.tileset = MetaGameState.points[num].x;
			num = (num + 1) % MetaGameState.points.Length;
		}
		this.RegenIslands();
		this.gamestate.StripDecorations(MetaGameState.points[0]);
	}

	// Token: 0x06000ABB RID: 2747 RVA: 0x000362F0 File Offset: 0x000346F0
	public int CountBBQS()
	{
		int num = 0;
		foreach (string text in this.islandnames)
		{
			GameState gameState = this.islands[text];
			foreach (Entity entity in gameState.entities)
			{
				if (entity.type == EntType.bbq)
				{
					num++;
				}
			}
		}
		return num;
	}

	// Token: 0x04000685 RID: 1669
	public string name;

	// Token: 0x04000686 RID: 1670
	public GameState gamestate;

	// Token: 0x04000687 RID: 1671
	public List<string> islandnames = new List<string>();

	// Token: 0x04000688 RID: 1672
	public StringDictionary<GameState> islands = new StringDictionary<GameState>();

	// Token: 0x04000689 RID: 1673
	public StringDictionary<Coord> offsets = new StringDictionary<Coord>();

	// Token: 0x0400068A RID: 1674
	public StringDictionary<List<KeyValuePair<Coord, Direction>>> sausagepositions = new StringDictionary<List<KeyValuePair<Coord, Direction>>>();

	// Token: 0x0400068B RID: 1675
	public StringDictionary<KeyValuePair<Coord, Direction>> playerpositions = new StringDictionary<KeyValuePair<Coord, Direction>>();

	// Token: 0x0400068C RID: 1676
	public StringDictionary<List<string>> templedat = new StringDictionary<List<string>>();

	// Token: 0x0400068D RID: 1677
	public StringDictionary<IslandMask> islandmasks = new StringDictionary<IslandMask>();

	// Token: 0x0400068E RID: 1678
	public StringDictionary<StringDictionary<bool[,]>> projectioncompatibilities = new StringDictionary<StringDictionary<bool[,]>>();

	// Token: 0x0400068F RID: 1679
	public StringDictionary<IntDictionary<List<Coord>>> coastdat = new StringDictionary<IntDictionary<List<Coord>>>();

	// Token: 0x04000690 RID: 1680
	public StringDictionary<IntDictionary<List<Coord>>> splashdat = new StringDictionary<IntDictionary<List<Coord>>>();

	// Token: 0x04000691 RID: 1681
	public StringDictionary<List<Coord>> bbqdat = new StringDictionary<List<Coord>>();

	// Token: 0x04000692 RID: 1682
	public StringDictionary<List<KeyValuePair<Coord, int>>> treedat = new StringDictionary<List<KeyValuePair<Coord, int>>>();

	// Token: 0x04000693 RID: 1683
	public Coord startpos;

	// Token: 0x04000694 RID: 1684
	public int version = 1;

	// Token: 0x04000695 RID: 1685
	private static Coord[] points = new Coord[]
	{
		new Coord(0, 0, 0),
		new Coord(0, 3, 0),
		new Coord(0, 12, 7),
		new Coord(1, 0, 0),
		new Coord(2, 0, 0),
		new Coord(3, 0, 1),
		new Coord(3, 2, 1),
		new Coord(3, 2, 2)
	};

	// Token: 0x020000A8 RID: 168
	public class SomeObject
	{
		// Token: 0x170003CD RID: 973
		// (get) Token: 0x06000AC3 RID: 2755 RVA: 0x000364AC File Offset: 0x000348AC
		// (set) Token: 0x06000AC4 RID: 2756 RVA: 0x000364B4 File Offset: 0x000348B4
		public int SimpleInt { get; set; }

		// Token: 0x170003CE RID: 974
		// (get) Token: 0x06000AC5 RID: 2757 RVA: 0x000364BD File Offset: 0x000348BD
		// (set) Token: 0x06000AC6 RID: 2758 RVA: 0x000364C5 File Offset: 0x000348C5
		public Fraction SimpleDateTime { get; set; }

		// Token: 0x170003CF RID: 975
		// (get) Token: 0x06000AC7 RID: 2759 RVA: 0x000364CE File Offset: 0x000348CE
		// (set) Token: 0x06000AC8 RID: 2760 RVA: 0x000364D6 File Offset: 0x000348D6
		public string SimpleString { get; set; }
	}

	// Token: 0x020000A9 RID: 169
	public enum SimpleEnum
	{
		// Token: 0x0400069F RID: 1695
		One,
		// Token: 0x040006A0 RID: 1696
		Two,
		// Token: 0x040006A1 RID: 1697
		Three
	}
}
