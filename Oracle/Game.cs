using System;
using System.Collections.Generic;

public static partial class Game {
  const double TICK = 0.15;

  public const long UNITS_PER_TICK = 1000;

  public static long ResolveMove(GameState gs) {
    // this.exitSequence = false;
    // this.ProcessSparks();
    double ticklength = TICK;
    // this.cameraperspectivetarget = this.cameraperspectivetargetnormal;
    long units = 0;
    for (;;)
    {
      // this.cameraperspectivetarget = this.cameraperspectivetargetnormal;
      double moveticklength = (double)gs.MoveTickLength();
      double maxspeed = (double)gs.MaxSpeed(true);
      double playermoveticklength = 1.0;
      if (gs.player != null && gs.player.moving && (double)gs.player.movement.speed > 0.8)
      {
        playermoveticklength = 1.0 / (double)gs.player.movement.speed;
        for (int m = 0; m < gs.movements.Count; m++)
        {
          Movement movement = gs.movements[m];
          if (movement.target != gs.player && movement.translation && movement.direction.Horizontal())
          {
            ticklength = TICK;
            break;
          }
        }
      }
      else
      {
        ticklength = TICK;
        playermoveticklength = 0.5;
        List<Entity> list = gs.OverWaterMovements();
        bool allOffScreen = list.Count == 0; // originally 'flag'
        // for (int n = 0; n < list.Count; n++)
        // {
        //   Entity entity = list[n];
        //   GameObject dynamicObject = this.GetDynamicObject(entity.id);
        //   if (!(dynamicObject == null))
        //   {
        //     Renderer component = dynamicObject.GetComponent<Renderer>();
        //     bool flag2;
        //     if (component != null)
        //     {
        //       flag2 = component.isVisible;
        //     }
        //     else
        //     {
        //       Vector3 vector = this.cam.WorldToViewportPoint(dynamicObject.transform.position);
        //       flag2 = vector.x >= 0f && vector.x <= 1f && vector.y >= 0f && vector.y <= 1f && vector.z > 0f;
        //     }
        //     if (flag2)
        //     {
        //       flag = false;
        //       break;
        //     }
        //   }
        // }
        if (allOffScreen)
        {
          ticklength = 0.010000001;
        }
        else if (gs.pushestotry != 0 && Math.Abs(gs.pushestotry) < 7)
        {
          ticklength = 0.075;
        }
      }
      if (maxspeed > 1.2 && playermoveticklength > 0.8)
      {
        ticklength = Math.Max(ticklength, TICK / moveticklength);
        // this.cameraperspectivetarget = this.cameraperspectivetargetmax;
      }
      if (gs.pushestotry >= 19 || (gs.pushestotry < 0 && gs.pushestotry >= -3) || gs.donelowering == 1)
      {
        ticklength = 0.45000002;
      }
      // if (this.gameover)
      // {
      // }
      // yield return Yielders.GetSecondsFloat(this.ticklength * moveticklength);
      units += (long)Math.Round(ticklength / TICK * moveticklength * UNITS_PER_TICK);
      // if (!this.bluespawnanim)
      // {
      //   iTween.Stop();
      // }
      gs.ContinueAutomatic();
      // if (this.gamestate.pushestotry != 0 && this.gamestate.waslowering)
      // {
      //   base.StartCoroutine("DisplayIslandName");
      //   this.gamestate.waslowering = false;
      // }
      // if (this.gamestate.sausageexplosions.Count > 0)
      // {
      //   this.exitSequence = true;
      //   yield return Yielders.GetSecondsFloat(0.15f);
      //   this.player_anim.Stop();
      //   this.player_anim.Play("idle");
      //   yield return Yielders.GetSecondsFloat(0.15f);
      //   GhostScript petag = this.playergo.GetComponent<GhostScript>();
      //   for (int num = 0; num < petag.particlesystems.Length; num++)
      //   {
      //     ParticleSystem particleSystem = petag.particlesystems[num];
      //     particleSystem.enableEmission = true;
      //   }
      //   if (!this.particleFizzleSound.isPlaying)
      //   {
      //     this.particleFizzleSound.volume = 0.1f;
      //     this.particleFizzleSound.Play();
      //   }
      //   this.SetEidolonStates();
      //   this.exploding = true;
      //   for (int i = 0; i < this.gamestate.sausageexplosions.Count; i++)
      //   {
      //     Coord coord = this.gamestate.sausageexplosions[i];
      //     Direction dir = this.gamestate.sausagedirs[i];
      //     Vector3 vec = coord.ToVector(false);
      //     IEnumerator enumerator = this.models.transform.GetEnumerator();
      //     try
      //     {
      //       while (enumerator.MoveNext())
      //       {
      //         object obj = enumerator.Current;
      //         Transform transform = (Transform)obj;
      //         if (Vector3.SqrMagnitude(transform.transform.position - vec) < 0.01f)
      //         {
      //           GameObject gameObject = transform.gameObject;
      //           EntityTag component2 = gameObject.GetComponent<EntityTag>();
      //           if (component2.type == EntType.sausage)
      //           {
      //             if (this.sigil.transform != null && this.sigil.transform.parent != null && this.sigil.transform.parent.parent == gameObject.transform)
      //             {
      //               this.sigil.transform.parent = null;
      //             }
      //             global::UnityEngine.Object.DestroyImmediate(gameObject);
      //             this.modeldict.Remove(component2.id);
      //           }
      //         }
      //       }
      //     }
      //     finally
      //     {
      //       IDisposable disposable;
      //       if ((disposable = enumerator as IDisposable) != null)
      //       {
      //         disposable.Dispose();
      //       }
      //     }
      //     GameObject go = global::UnityEngine.Object.Instantiate<GameObject>(this.sausagedestroycontainer, coord.ToVector(false), dir.ToQuat());
      //     global::UnityEngine.Object.Destroy(go, 4f);
      //     this.asource.PlayOneShot(this.sfx.sfx_eatsounds[global::UnityEngine.Random.Range(0, this.sfx.sfx_eatsounds.Length)], this.eatsoundvol);
      //     if (!this.gamestate.won)
      //     {
      //       yield return Yielders.GetSecondsFloat(this.eatsounddelay);
      //     }
      //   }
      // }
      // this.exitSequence = false;
      // this.exploding = false;
      // this.gamestate.sausageexplosions.Clear();
      // this.gamestate.sausagedirs.Clear();
      // this.ProcessSparks();
      // this.UpdateCameraOffsets();
      // this.elapsed = 0f;
      // if (this.gamestate.regenfork)
      // {
      //   if (this.gamestate.player.Extended())
      //   {
      //     this.playerhandl1.GetComponent<Renderer>().enabled = true;
      //     this.playerhandr1.GetComponent<Renderer>().enabled = true;
      //     this.playerhandl2.GetComponent<Renderer>().enabled = false;
      //     this.playerhandr2.GetComponent<Renderer>().enabled = false;
      //     this.playerfork.GetComponent<Renderer>().enabled = true;
      //   }
      //   else
      //   {
      //     this.playerhandl1.GetComponent<Renderer>().enabled = false;
      //     this.playerhandr1.GetComponent<Renderer>().enabled = false;
      //     this.playerhandl2.GetComponent<Renderer>().enabled = true;
      //     this.playerhandr2.GetComponent<Renderer>().enabled = true;
      //     this.playerfork.GetComponent<Renderer>().enabled = false;
      //   }
      //   if (this.playerfork.GetComponent<Renderer>().enabled && this.forkparent != null)
      //   {
      //     int id = this.forkgo.GetComponent<EntityTag>().id;
      //     this.modeldict.Remove(id);
      //     global::UnityEngine.Object.Destroy(this.forkpart);
      //     global::UnityEngine.Object.Destroy(this.forkgo);
      //     this.forkpart = null;
      //     this.forkgo = null;
      //     this.forkparent = null;
      //   }
      //   this.gamestate.regenfork = false;
      // }
      // if (this.gamestate.addfork)
      // {
      //   if (this.gamestate.fork != null)
      //   {
      //     this.RedrawModel(this.gamestate.fork);
      //   }
      //   this.gamestate.addfork = false;
      // }
      // if (this.gamestate.forkfork)
      // {
      //   if (this.gamestate.fork != null)
      //   {
      //     Entity entity2 = this.gamestate.fork;
      //     if (entity2.movement == null && this.forkparent == null && entity2.Laden() && entity2.LadenTarget().movement == null)
      //     {
      //       GameObject gameObject2 = this.forkgo;
      //       GameObject dynamicObject2 = this.GetDynamicObject(entity2.stuckto);
      //       if (gameObject2.transform.childCount == 0)
      //       {
      //         int id2 = this.forkgo.GetComponent<EntityTag>().id;
      //         this.modeldict.Remove(id2);
      //         global::UnityEngine.Object.Destroy(this.forkgo);
      //         this.forkgo = null;
      //         this.RedrawModel(this.gamestate.fork);
      //         gameObject2 = this.forkgo;
      //       }
      //       this.forkpart = gameObject2.transform.GetChild(0).gameObject;
      //       Transform transform2 = dynamicObject2.transform.Find("zCylinder2");
      //       this.forkpart.transform.parent = transform2;
      //       this.forkparent = dynamicObject2.transform;
      //       this.gamestate.forkfork = false;
      //     }
      //   }
      //   else
      //   {
      //     if (this.forkparent != null)
      //     {
      //       int id3 = this.forkgo.GetComponent<EntityTag>().id;
      //       this.modeldict.Remove(id3);
      //       global::UnityEngine.Object.Destroy(this.forkpart);
      //       global::UnityEngine.Object.Destroy(this.forkgo);
      //       this.forkpart = null;
      //       this.forkgo = null;
      //     }
      //     this.gamestate.forkfork = false;
      //   }
      // }
      if (!gs.Moving())
      {
        // this.SetText();
        // this.CheckBriefingText();
        // if (this.gamestate.donelowering != 0)
        // {
        //   this.gamestate.donelowering = 0;
        //   this.TakeRestartSnapshot();
        // }
        // this.cameraperspectivetarget = this.cameraperspectivetargetnormal;
        // if (this.lastinputisland != this.gamestate.pushtargetlevel)
        // {
        //   this.lastinputisland = this.gamestate.pushtargetlevel;
        //   if (this.gamestate.pushtargetlevel == string.Empty)
        //   {
        //     this.inputreplaystrings.inputstring.Append("(World)");
        //   }
        //   else
        //   {
        //     this.inputreplaystrings.inputstring.Append("(" + this.gamestate.pushtargetlevel + ")");
        //   }
        // }
        // if (this.initialGhost != null)
        // {
        //   this.initialGhost.SetActive(this.gamestate.won);
        // }
        // if (this.gamestate.won && Game.loadedLevelName == "WorldExplore" && this.gamestate.player.pos == this.gamestate.metagame.startpos && this.gamestate.player.direction == Direction.North)
        // {
        //   break;
        // }
        // if (this.gamestate.haveevercookedall && this.gamestate.haveevercookedall != this.oldHaveevercookedall)
        // {
        //   this.oldHaveevercookedall = true;
        //   this.player_anim.Stop();
        //   this.player_anim.Play("idle");
        //   this.bluespawnanim = true;
        //   this.cameratrack.enabled = false;
        //   this.initialGhost.SetActive(false);
        //   Vector3 campos = this.cam.transform.position;
        //   Vector3 camtargetpos = this.initialGhost.transform.position + this.cameratrack.displacement;
        //   float dist = Vector3.Distance(campos, camtargetpos);
        //   float lerptime = 1f;
        //   WaitForSeconds WaitLerp = new WaitForSeconds(lerptime);
        //   iTween.MoveTo(this.cam.gameObject, iTween.Hash(new object[]
        //   {
        //     "position",
        //     camtargetpos,
        //     "time",
        //     lerptime,
        //     "easetype",
        //     iTween.EaseType.easeInOutQuad
        //   }));
        //   yield return WaitLerp;
        //   this.initialGhost.SetActive(true);
        //   this.eidolonparticles.transform.position = this.initialGhost.transform.position;
        //   this.eidolonparticles.transform.rotation = this.initialGhost.transform.rotation;
        //   this.eidolonparticles.Emit(10);
        //   this.asource.PlayOneShot(this.sfx.sfx_exitappear[this.gamestate.pushtargetlevel.GetHashCode() % this.sfx.sfx_exitappear.Length], this.sfx.exitappearvol);
        //   yield return WaitLerp;
        //   iTween.MoveTo(this.cam.gameObject, iTween.Hash(new object[]
        //   {
        //     "position",
        //     campos,
        //     "time",
        //     lerptime,
        //     "easetype",
        //     iTween.EaseType.easeInOutQuad
        //   }));
        //   yield return WaitLerp;
        //   this.cameratrack.enabled = true;
        //   this.bluespawnanim = false;
        // }
        if (!gs.Moving())
        {
          goto Block_66;
        }
      }
      // else
      // {
      //   if (this.gamestate.sausagelost)
      //   {
      //     this.SetText();
      //   }
      //   if (this.gamestate.pushestotry == 10 && !this.gamestate.overworld)
      //   {
      //     this.TakeExitSnapshot();
      //   }
      // }
    }
    // Game.endingsequence = true;
    // base.StartCoroutine("FadeSynth");
    // this.particleFizzleSound.Stop();
    // this.initialGhost.SetActive(false);
    // this.player_anim.Stop();
    // this.player_anim.Play("idle");
    // this.bluespawnanim = true;
    // this.cameratrack.enabled = false;
    // GhostScript gs = this.playergo.GetComponent<GhostScript>();
    // foreach (ParticleSystem particleSystem2 in gs.particlesystems)
    // {
    //   particleSystem2.enableEmission = true;
    // }
    // Vector3 campos2 = this.cam.transform.position;
    // float lerptime2 = 2f;
    // yield return new WaitForSeconds(lerptime2);
    // for (int j = 0; j < this.gamestate.entities.Count; j++)
    // {
    //   Entity e = this.gamestate.entities[j];
    //   if (e.type == EntType.sausage)
    //   {
    //     GameObject go_s = this.modeldict[e.id];
    //     Vector3 camtargetpos2 = go_s.transform.position + this.cameratrack.displacement;
    //     float dist2 = Vector3.Distance(campos2, camtargetpos2);
    //     iTween.MoveTo(this.cam.gameObject, iTween.Hash(new object[]
    //     {
    //       "position",
    //       camtargetpos2,
    //       "time",
    //       lerptime2,
    //       "easetype",
    //       iTween.EaseType.easeInOutQuad
    //     }));
    //     yield return new WaitForSeconds(lerptime2);
    //     EntityTag etag = go_s.GetComponent<EntityTag>();
    //     if (etag.type == EntType.sausage)
    //     {
    //       if (this.sigil.transform != null && this.sigil.transform.parent != null && this.sigil.transform.parent.parent == go_s.transform)
    //       {
    //         this.sigil.transform.parent = null;
    //       }
    //       global::UnityEngine.Object.DestroyImmediate(go_s);
    //       this.modeldict.Remove(etag.id);
    //     }
    //     GameObject go2 = global::UnityEngine.Object.Instantiate<GameObject>(this.sausagedestroycontainer, e.pos.ToVector(false), e.direction.ToQuat());
    //     global::UnityEngine.Object.Destroy(go2, 4f);
    //     this.asource.PlayOneShot(this.sfx.sfx_eatsounds[global::UnityEngine.Random.Range(0, this.sfx.sfx_eatsounds.Length)], this.eatsoundvol);
    //     yield return Yielders.GetSecondsFloat(this.eatsounddelay);
    //   }
    // }
    // iTween.MoveTo(this.cam.gameObject, iTween.Hash(new object[]
    // {
    //   "position",
    //   campos2,
    //   "time",
    //   lerptime2,
    //   "easetype",
    //   iTween.EaseType.easeInOutQuad
    // }));
    // yield return new WaitForSeconds(lerptime2);
    // this.bluespawnanim = false;
    // foreach (ParticleSystem particleSystem3 in gs.particlesystems)
    // {
    //   particleSystem3.enableEmission = false;
    // }
    // this.initialGhost.SetActive(false);
    // this.gamestate.pushtargetlevel = "start";
    // this.gamestate.pushestotry = 22;
    // this.ticklength = 0.45000002f;
    // CameraTrack.Shake(0.1f, 0.5f);
    // Rumbler.BigRumble();
    // this.asource.PlayOneShot(this.sfx.sfx_leave, this.sfx.leaveVol);
    // this.asource.PlayOneShot(this.sfx.sfx_eidolonStep, this.sfx.eidolonStepVol);
    // for (int k = 0; k < 12; k++)
    // {
    //   if (k > 6)
    //   {
    //     this.gamestate.pushtargetlevel = string.Empty;
    //   }
    //   this.elapsed = 0f;
    //   this.gamestate.ContinueAutomatic();
    //   yield return new WaitForSeconds(this.ticklength * (float)this.gamestate.MoveTickLength());
    // }
    // yield return new WaitForSeconds(4f);
    // this.gamestate.pushestotry = -22;
    // this.playergo.SetActive(false);
    // this.playerFollow.target = this.playergo;
    // this.playerFollow.gameObject.SetActive(true);
    // CameraTrack.Shake(0.1f, 0.5f);
    // Rumbler.BigRumble();
    // this.asource.PlayOneShot(this.sfx.sfx_eidolonStep, this.sfx.eidolonStepVol);
    // for (int l = 0; l < 12; l++)
    // {
    //   if (l > 6)
    //   {
    //     this.gamestate.pushtargetlevel = string.Empty;
    //   }
    //   this.elapsed = 0f;
    //   this.gamestate.ContinueAutomatic();
    //   yield return new WaitForSeconds(this.ticklength * (float)this.gamestate.MoveTickLength());
    // }
    // yield return new WaitForSeconds(4f);
    // this.bluespawnanim = true;
    // AutoFade.LoadLevel("CreditsScene", 10f);
    // this.leaving = true;
    // yield break;
    Block_66:
    return units;
  }
}
