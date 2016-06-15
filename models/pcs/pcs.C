#include "pcs.h"

#define max fmax

double
Pi_Distribution(double n, double N)
{
  double          i;
  double          nfactorial;

  nfactorial = 1.0;
  for (i = 1.0; i <= n; i += 1.0)
    nfactorial *= i;
  return ((pow(N, n) * exp(-N)) / nfactorial);
}

int
GenInitPortables(tw_lp * lp)
{
  return ((int)BIG_N);
}

tw_lpid Cell_ComputeMove( tw_lpid lpid, int direction )
{
  tw_lpid lpid_x, lpid_y;
  tw_lpid n_x, n_y;
  tw_lpid dest_lpid;

  lpid_y = lpid / NUM_CELLS_X;
  lpid_x = lpid - (lpid_y * NUM_CELLS_X);

  switch( direction )
  {
    case 0:
      n_x = ((lpid_x - 1) + NUM_CELLS_X) % NUM_CELLS_X;
      n_y = lpid_y;
      break;

    case 1:
      n_x = (lpid_x + 1) % NUM_CELLS_X;
      n_y = lpid_y;
      break;

    case 2:
      n_x = lpid_x;
      n_y = ((lpid_y - 1) + NUM_CELLS_Y) % NUM_CELLS_Y;
      break;

    case 3:
      n_x = lpid_x;
      n_y = (lpid_y + 1) % NUM_CELLS_Y;
      break;

    default:
      tw_error( TW_LOC, "Bad direction value \n");
  }

  dest_lpid = (tw_lpid) (n_x + (n_y * NUM_CELLS_X));
  // printf("ComputeMove: Src LP %llu (%d, %d), Dir %u, Dest LP %llu (%d, %d)\n", lpid, lpid_x, lpid_y, direction, dest_lpid, n_x, n_y);
  return( dest_lpid );
}

// Return LPChare index based on GID
unsigned pcs_grid_map (tw_lpid gid) {
  int cell_vp_x = (NUM_CELLS_X / NUM_VP_X);
  int cell_vp_y = (NUM_CELLS_Y / NUM_VP_Y);
  int local_x = ((gid % NUM_CELLS_X) / cell_vp_x);
  local_x %= NUM_VP_X;
  int local_y = ((gid / NUM_CELLS_X) /cell_vp_y);
  local_y %= NUM_VP_Y;
  return (local_y * NUM_VP_X + local_x);
}

// Return local index based on GID
tw_lpid pcs_local_map (tw_lpid gid) {
  int cell_vp_x = (NUM_CELLS_X / NUM_VP_X);
  int cell_vp_y = (NUM_CELLS_Y / NUM_VP_Y);
  int local_x = (gid % NUM_CELLS_X) % cell_vp_x;
  int local_y = (gid / NUM_CELLS_X) % cell_vp_y;
  return (local_y * cell_vp_x + local_x);
}

Min_t
Cell_MinTS(struct Msg_Data *M)
{
  if (M->CompletionCallTS < M->NextCallTS)
  {
    if (M->CompletionCallTS < M->MoveCallTS)
      return (COMPLETECALL);
    else
      return (MOVECALL);
  } else {
    if (M->NextCallTS < M->MoveCallTS)
      return (NEXTCALL);
    else
      return (MOVECALL);
  }
}

void
Cell_StartUp(struct State *SV, tw_lp * lp)
{
  tw_lpid currentcell = 0, newcell = 0;
  int             i, dest_index = 0;
  tw_stime          ts;

  struct Msg_Data TMsg;
  struct Msg_Data * TWMsg;
  tw_event *CurEvent;

  SV->Normal_Channels = MAX_NORMAL_CHANNELS;
  SV->Reserve_Channels = MAX_RESERVE_CHANNELS;
  SV->Portables_In = 0;
  SV->Portables_Out = 0;
  SV->Call_Attempts = 0;
  SV->Channel_Blocks = 0;
  SV->Handoff_Blocks = 0;
  SV->Busy_Lines = 0;
  SV->Handoff_Blocks = 0;
  SV->CellLocationX = lp->gid % NUM_CELLS_X;
  SV->CellLocationY = lp->gid / NUM_CELLS_X;

  if (SV->CellLocationX >= NUM_CELLS_X ||
      SV->CellLocationY >= NUM_CELLS_Y) {
    tw_error(TW_LOC, "Cell_StartUp: Bad CellLocations %d %d \n",
        SV->CellLocationX, SV->CellLocationY);
  }
  SV->Portables_In = GenInitPortables(lp);

  for (i = 0; i < SV->Portables_In; i++) {
    TMsg.CompletionCallTS = HUGE_VAL;

    TMsg.MoveCallTS = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
    TMsg.NextCallTS = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);

    switch (Cell_MinTS(&TMsg))
    {
      case COMPLETECALL:
        tw_error(TW_LOC, "APP_ERROR(StartUp): CompletionCallTS(%lf) Is Min \n",
            TMsg.CompletionCallTS);
        break;

      case NEXTCALL:
        ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
        CurEvent = tw_event_new(lp->gid, ts, lp);
        TWMsg = (struct Msg_Data *) tw_event_data(CurEvent);
        TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
        TWMsg->MoveCallTS = TMsg.MoveCallTS;
        TWMsg->NextCallTS = TMsg.NextCallTS;
        TWMsg->MethodName = NEXTCALL_METHOD;
        tw_event_send(CurEvent);
        break;

      case MOVECALL:
        newcell = lp->gid;
        while (TMsg.MoveCallTS < TMsg.NextCallTS)
        {
          double          result;

          currentcell = newcell;
          dest_index = tw_rand_integer(lp->rng, 0, 3);
          newcell = Cell_ComputeMove( currentcell, dest_index ); // Neighbors[currentcell][dest_index];
          result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
          TMsg.MoveCallTS += result;
        }

        ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
        CurEvent = tw_event_new(currentcell, ts, lp);
        TWMsg = (Msg_Data *) tw_event_data(CurEvent);
        TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
        TWMsg->MoveCallTS = TMsg.MoveCallTS;
        TWMsg->NextCallTS = TMsg.NextCallTS;
        TWMsg->MethodName = NEXTCALL_METHOD;
        tw_event_send(CurEvent);
        break;
    }
  }
}

void
Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             done, dest_index = 0;
  int             currentcell = 0, newcell = 0;
  tw_stime          ts;
  struct Msg_Data TMsg;
  struct Msg_Data *TWMsg;
  tw_event       *CurEvent;
  double          result;

  TMsg.MethodName = M->MethodName;
  TMsg.ChannelType = M->ChannelType;
  TMsg.CompletionCallTS = M->CompletionCallTS;
  TMsg.NextCallTS = M->NextCallTS;
  TMsg.MoveCallTS = M->MoveCallTS;

  SV->Call_Attempts++;

  if ((CV->c1 = NORM_CH_BUSY))
  {
    SV->Channel_Blocks++;
    result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
    TMsg.NextCallTS += result;

    switch (Cell_MinTS(&TMsg))
    {
      case COMPLETECALL:
        tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
            TMsg.CompletionCallTS);
        break;

      case NEXTCALL:
        ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
        CurEvent = tw_event_new(lp->gid, ts, lp);
        TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
        TWMsg->MethodName = TMsg.MethodName;
        TWMsg->ChannelType = TMsg.ChannelType;
        TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
        TWMsg->NextCallTS = TMsg.NextCallTS;
        TWMsg->MoveCallTS = TMsg.MoveCallTS;
        TWMsg->MethodName = NEXTCALL_METHOD;
        tw_event_send(CurEvent);
        break;

      case MOVECALL:
        newcell = lp->gid;
        while (TMsg.MoveCallTS < TMsg.NextCallTS)
        {
          M->RC.wl1++;
          currentcell = newcell;
          dest_index = tw_rand_integer(lp->rng, 0, 3);
          newcell = Cell_ComputeMove( currentcell, dest_index ); //Neighbors[currentcell][dest_index];
          result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
          TMsg.MoveCallTS += result;
        }

        ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
        CurEvent = tw_event_new((currentcell), ts, lp);
        TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
        TWMsg->MethodName = TMsg.MethodName;
        TWMsg->ChannelType = TMsg.ChannelType;
        TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
        TWMsg->NextCallTS = TMsg.NextCallTS;
        TWMsg->MoveCallTS = TMsg.MoveCallTS;
        TWMsg->MethodName = NEXTCALL_METHOD;
        tw_event_send(CurEvent);
        break;
    }
  } else {
    SV->Normal_Channels--;
    TMsg.ChannelType = NORMAL_CH;

    result = tw_rand_exponential(lp->rng, CALL_TIME_MEAN);
    TMsg.CompletionCallTS = result + TMsg.NextCallTS;
    result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
    TMsg.NextCallTS += result;
    done = 0;
    while (1)
    {
      switch (Cell_MinTS(&TMsg))
      {
        case COMPLETECALL:
          ts = max(0.0, TMsg.CompletionCallTS - tw_now(lp));
          CurEvent = tw_event_new(lp->gid, ts, lp);
          TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
          TWMsg->MethodName = TMsg.MethodName;
          TWMsg->ChannelType = TMsg.ChannelType;
          TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
          TWMsg->NextCallTS = TMsg.NextCallTS;
          TWMsg->MoveCallTS = TMsg.MoveCallTS;
          TWMsg->MethodName = COMPLETIONCALL_METHOD;
          tw_event_send(CurEvent);
          done = 1;
          break;

        case NEXTCALL:
          M->RC.wl1++;

          SV->Busy_Lines++;
          SV->Call_Attempts++;
          result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
          TMsg.NextCallTS += result;
          break;

        case MOVECALL:
          ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
          CurEvent = tw_event_new(lp->gid, ts, lp);
          TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
          TWMsg->MethodName = TMsg.MethodName;
          TWMsg->ChannelType = TMsg.ChannelType;
          TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
          TWMsg->NextCallTS = TMsg.NextCallTS;
          TWMsg->MoveCallTS = TMsg.MoveCallTS;
          TWMsg->MethodName = MOVECALLOUT_METHOD;
          tw_event_send(CurEvent);
          done = 1;
          break;
      }
      if (done)
        break;
    }
  }
}

void
Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             dest_index = 0;
  int             currentcell = 0, newcell = 0;
  struct Msg_Data TMsg;
  double          result;
  tw_stime          ts;
  struct Msg_Data *TWMsg;
  tw_event       *CurEvent;

  TMsg.MethodName = M->MethodName;
  TMsg.ChannelType = M->ChannelType;
  TMsg.CompletionCallTS = HUGE_VAL;
  TMsg.NextCallTS = M->NextCallTS;
  TMsg.MoveCallTS = M->MoveCallTS;

  if ((CV->c1 = (NORMAL_CH == M->ChannelType)))
    SV->Normal_Channels++;
  else if ((CV->c2 = RESERVE == M->ChannelType))
    SV->Reserve_Channels++;
  else {
    tw_error(TW_LOC, "APP_ERROR(2): CompletionCall: Bad ChannelType(%d) \n",
        M->ChannelType);
  }
  if (SV->Normal_Channels > MAX_NORMAL_CHANNELS ||
      SV->Reserve_Channels > MAX_RESERVE_CHANNELS)
  {
    tw_error(TW_LOC, "APP_ERROR(3): Normal_Channels(%d) > MAX %d OR Reserve_Channels(%d) > MAX %d \n",
        SV->Normal_Channels, MAX_NORMAL_CHANNELS, SV->Reserve_Channels,
        MAX_RESERVE_CHANNELS);
  }
  TMsg.ChannelType = NONE;

  switch (Cell_MinTS(&TMsg))
  {
    case COMPLETECALL:
      tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
          TMsg.CompletionCallTS);
      break;

    case NEXTCALL:
      ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
      CurEvent = tw_event_new(lp->gid, ts, lp);
      TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

      TWMsg->MethodName = TMsg.MethodName;
      TWMsg->ChannelType = TMsg.ChannelType;
      TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
      TWMsg->NextCallTS = TMsg.NextCallTS;
      TWMsg->MoveCallTS = TMsg.MoveCallTS;

      TWMsg->MethodName = NEXTCALL_METHOD;
      tw_event_send(CurEvent);
      break;

    case MOVECALL:
      newcell = lp->gid;
      while (TMsg.MoveCallTS < TMsg.NextCallTS)
      {
        M->RC.wl1++;
        currentcell = newcell;
        dest_index = tw_rand_integer(lp->rng, 0, 3);
        newcell = Cell_ComputeMove( currentcell, dest_index ); //Neighbors[currentcell][dest_index];

        result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
        TMsg.MoveCallTS += result;
      }
      ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
      CurEvent = tw_event_new((currentcell), ts, lp);
      TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
      TWMsg->MethodName = TMsg.MethodName;
      TWMsg->ChannelType = TMsg.ChannelType;
      TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
      TWMsg->NextCallTS = TMsg.NextCallTS;
      TWMsg->MoveCallTS = TMsg.MoveCallTS;
      TWMsg->MethodName = NEXTCALL_METHOD;
      tw_event_send(CurEvent);
      break;
  }
}

void
Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             done, dest_index = 0;
  int             currentcell = 0, newcell = 0;
  struct Msg_Data TMsg;
  double          result;
  tw_stime          ts;
  tw_event       *CurEvent;

  struct Msg_Data *TWMsg;

  TMsg.MethodName = M->MethodName;
  TMsg.ChannelType = M->ChannelType;
  TMsg.CompletionCallTS = M->CompletionCallTS;
  TMsg.NextCallTS = M->NextCallTS;
  result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
  TMsg.MoveCallTS = M->MoveCallTS + result;
  if ((CV->c1 = (TMsg.CompletionCallTS != HUGE_VAL)))
  {
    if ((CV->c2 = (NORM_CH_BUSY && RESERVE_CH_BUSY)))
    {
      SV->Handoff_Blocks++;
      TMsg.CompletionCallTS = HUGE_VAL;

      switch (Cell_MinTS(&TMsg))
      {
        case COMPLETECALL:
          tw_error(TW_LOC, "APP_ERROR(NextCall): CompletionCallTS(%lf) Is Min \n",
              TMsg.CompletionCallTS);
          break;

        case NEXTCALL:
          ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
          CurEvent = tw_event_new(lp->gid, ts, lp);
          TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

          TWMsg->MethodName = TMsg.MethodName;
          TWMsg->ChannelType = TMsg.ChannelType;
          TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
          TWMsg->NextCallTS = TMsg.NextCallTS;
          TWMsg->MoveCallTS = TMsg.MoveCallTS;

          TWMsg->MethodName = NEXTCALL_METHOD;
          tw_event_send(CurEvent);
          break;

        case MOVECALL:
          newcell = lp->gid;
          while (TMsg.MoveCallTS < TMsg.NextCallTS)
          {
            M->RC.wl1++;
            currentcell = newcell;
            dest_index = tw_rand_integer(lp->rng, 0, 3);
            newcell = Cell_ComputeMove( currentcell, dest_index ); // Neighbors[currentcell][dest_index];
            result = tw_rand_exponential(lp->rng, MOVE_CALL_MEAN);
            TMsg.MoveCallTS += result;
          }
          ts = max(0.0, TMsg.NextCallTS - tw_now(lp));
          CurEvent = tw_event_new((currentcell), ts, lp);
          TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
          TWMsg->MethodName = TMsg.MethodName;
          TWMsg->ChannelType = TMsg.ChannelType;
          TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
          TWMsg->NextCallTS = TMsg.NextCallTS;
          TWMsg->MoveCallTS = TMsg.MoveCallTS;
          TWMsg->MethodName = NEXTCALL_METHOD;
          tw_event_send(CurEvent);
          break;
      }
    } else {
      if ((CV->c3 = !NORM_CH_BUSY))
      {
        SV->Normal_Channels--;
        TMsg.ChannelType = NORMAL_CH;
      } else
      {
        SV->Reserve_Channels--;
        TMsg.ChannelType = RESERVE;
      }
      done = 0;
      while (1)
      {
        switch (Cell_MinTS(&TMsg))
        {
          case COMPLETECALL:
            ts = max(0.0, TMsg.CompletionCallTS - tw_now(lp));
            CurEvent = tw_event_new(lp->gid, ts, lp);
            TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

            TWMsg->MethodName = TMsg.MethodName;
            TWMsg->ChannelType = TMsg.ChannelType;
            TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
            TWMsg->NextCallTS = TMsg.NextCallTS;
            TWMsg->MoveCallTS = TMsg.MoveCallTS;

            TWMsg->MethodName = COMPLETIONCALL_METHOD;
            tw_event_send(CurEvent);
            done = 1;
            break;

          case NEXTCALL:
            M->RC.wl1++;
            SV->Busy_Lines++;
            SV->Call_Attempts++;
            result = tw_rand_exponential(lp->rng, NEXT_CALL_MEAN);
            TMsg.NextCallTS += result;
            break;

          case MOVECALL:
            ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
            CurEvent = tw_event_new(lp->gid, ts, lp);
            TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);

            TWMsg->MethodName = TMsg.MethodName;
            TWMsg->ChannelType = TMsg.ChannelType;
            TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
            TWMsg->NextCallTS = TMsg.NextCallTS;
            TWMsg->MoveCallTS = TMsg.MoveCallTS;

            TWMsg->MethodName = MOVECALLOUT_METHOD;
            tw_event_send(CurEvent);
            done = 1;
            break;
        }
        if (done)
          break;
      }
    }
  } else {
    tw_error(TW_LOC, "APP_ERROR(11): MoveCallIn: Got MoveCallIn Event W/O Call In Progress!! \n");
  }
}

void
Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             dest_index;
  int             newcell;
  struct Msg_Data TMsg;

  tw_stime          ts;

  struct Msg_Data *TWMsg;
  tw_event       *CurEvent;

  TMsg.MethodName = M->MethodName;
  TMsg.ChannelType = M->ChannelType;
  TMsg.CompletionCallTS = M->CompletionCallTS;
  TMsg.NextCallTS = M->NextCallTS;
  TMsg.MoveCallTS = M->MoveCallTS;

  if ((CV->c1 = TMsg.CompletionCallTS != HUGE_VAL))
  {
    if ((CV->c2 = NORMAL_CH == M->ChannelType))
      SV->Normal_Channels++;
    else if (RESERVE == M->ChannelType)
      SV->Reserve_Channels++;
    else {
      tw_error(TW_LOC, "APP_ERROR(7): MoveCallOut: Bad ChannelType(%d) \n",
          M->ChannelType);
    }
    TMsg.ChannelType = NONE;
  } else {
    tw_error(TW_LOC, "APP_ERROR(9): MoveCallOut: NOT IN CALL: SHOULD NOT BE HERE!! \n");
  }

  ts = max(0.0, TMsg.MoveCallTS - tw_now(lp));
  TMsg.MethodName = MOVECALLIN_METHOD;
  dest_index = tw_rand_integer(lp->rng, 0, 3);
  newcell = Cell_ComputeMove( lp->gid, dest_index ); //Neighbors[lp->gid][dest_index];
  CurEvent = tw_event_new((newcell), ts, lp);
  TWMsg = (struct Msg_Data *)tw_event_data(CurEvent);
  TWMsg->MethodName = TMsg.MethodName;
  TWMsg->ChannelType = TMsg.ChannelType;
  TWMsg->CompletionCallTS = TMsg.CompletionCallTS;
  TWMsg->NextCallTS = TMsg.NextCallTS;
  TWMsg->MoveCallTS = TMsg.MoveCallTS;
  tw_event_send(CurEvent);
}

void
Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
#ifdef LPTRACEON
  long            seeds[4];
#endif
  *(int *)CV = (int)0;
  M->RC.wl1 = 0;

  switch (M->MethodName)
  {
    case NEXTCALL_METHOD:
      Cell_NextCall(SV, CV, M, lp);
      break;
    case COMPLETIONCALL_METHOD:
      Cell_CompletionCall(SV, CV, M, lp);
      break;
    case MOVECALLIN_METHOD:
      Cell_MoveCallIn(SV, CV, M, lp);
      break;
    case MOVECALLOUT_METHOD:
      Cell_MoveCallOut(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d)\n",
          lp->gid, M->MethodName);
  }

#ifdef LPTRACEON
  rng_get_state(lp->gid, seeds);
  fprintf(LPTrace[lp->gid], "CE: Type %d: Time %f: %d %d %d %d \n", M->MethodName,
	  tw_now(lp), seeds[0], seeds[1], seeds[2], seeds[3]);
#endif
}

void
RC_Cell_NextCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             i;

  SV->Call_Attempts--;
  if (CV->c1)
  {
    SV->Channel_Blocks--;
    tw_rand_reverse_unif(lp->rng);
    for (i = 0; i < M->RC.wl1; i++)
    {
      tw_rand_reverse_unif(lp->rng);
      tw_rand_reverse_unif(lp->rng);
    }
  } else {
    SV->Normal_Channels++;
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);
    for (i = 0; i < M->RC.wl1; i++)
    {
      SV->Busy_Lines--;
      SV->Call_Attempts--;
      tw_rand_reverse_unif(lp->rng);
    }
  }
}

void
RC_Cell_CompletionCall(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             i;

  if (CV->c1)
    SV->Normal_Channels--;
  else if (CV->c2)
    SV->Reserve_Channels--;

  for (i = 0; i < M->RC.wl1; i++)
  {
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);
  }
}

void
RC_Cell_MoveCallIn(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  int             i;

  tw_rand_reverse_unif(lp->rng);
  if (CV->c1)
  {
    if (CV->c2)
    {
      SV->Handoff_Blocks--;

      for (i = 0; i < M->RC.wl1; i++)
      {
        tw_rand_reverse_unif(lp->rng);
        tw_rand_reverse_unif(lp->rng);
      }
    } else {
      if (CV->c3)
        SV->Normal_Channels++;
      else
        SV->Reserve_Channels++;
      for (i = 0; i < M->RC.wl1; i++)
      {
        SV->Busy_Lines--;
        SV->Call_Attempts--;
        tw_rand_reverse_unif(lp->rng);
      }
    }
  }
}

void
RC_Cell_MoveCallOut(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
  if (CV->c1)
  {
    if (CV->c2)
      SV->Normal_Channels--;
    else
      SV->Reserve_Channels--;
  }
  tw_rand_reverse_unif(lp->rng);
}


void
RC_Cell_EventHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp)
{
#ifdef LPTRACEON
  long            seeds[4];
#endif

  switch (M->MethodName)
    {
    case NEXTCALL_METHOD:
      RC_Cell_NextCall(SV, CV, M, lp);
      break;
    case COMPLETIONCALL_METHOD:
      RC_Cell_CompletionCall(SV, CV, M, lp);
      break;
    case MOVECALLIN_METHOD:
      RC_Cell_MoveCallIn(SV, CV, M, lp);
      break;
    case MOVECALLOUT_METHOD:
      RC_Cell_MoveCallOut(SV, CV, M, lp);
      break;
    }

#ifdef LPTRACEON
  rng_get_state(lp->gid, seeds);
  fprintf(LPTrace[lp->gid], "RC_CE: Type %d: %d %d %d %d \n", M->MethodName,
	  seeds[0], seeds[1], seeds[2], seeds[3]);
#endif
}


void
CellStatistics_CollectStats(struct State *SV, tw_lp * lp)
{
#ifdef CELL_STATS
  TWAppStats.Call_Attempts += SV->Call_Attempts;
  TWAppStats.Channel_Blocks += SV->Channel_Blocks;
  TWAppStats.Busy_Lines += SV->Busy_Lines;
  TWAppStats.Handoff_Blocks += SV->Handoff_Blocks;
  TWAppStats.Portables_In += SV->Portables_In;
  TWAppStats.Portables_Out += SV->Portables_Out;
#endif
}

void
Cell_CommitHandler(struct State *SV, tw_bf * CV, struct Msg_Data *M, tw_lp * lp) {}

#ifdef CELL_STATS
void
CellStatistics_Compute(struct CellStatistics *CS)
{
  CS->Blocking_Probability = ((double)CS->Channel_Blocks + (double)CS->Handoff_Blocks) /
    ((double)CS->Call_Attempts - (double)CS->Busy_Lines);
}

void
CellStatistics_Print(struct CellStatistics *CS)
{
  printf("Call Attempts......................................%d\n",
	 CS->Call_Attempts);
  printf("Channel Blocks.....................................%d\n",
	 CS->Channel_Blocks);
  printf("Busy Lines.........................................%d\n",
	 CS->Busy_Lines);
  printf("Handoff Blocks.....................................%d\n",
	 CS->Handoff_Blocks);
  printf("Portables In.......................................%d\n",
	 CS->Portables_In);
  printf("Portables Out......................................%d\n",
	 CS->Portables_Out);
  printf("Blocking Probability...............................%f\n",
	 CS->Blocking_Probability);
}
#endif

/******** Initialize_Appl *************************************************/

#define	TW_CELL	1

tw_lptype       mylps[] =
{
  {
    (init_f) Cell_StartUp,
    (event_f) Cell_EventHandler,
    (revent_f) RC_Cell_EventHandler,
    (final_f) CellStatistics_CollectStats,
    (commit_f) Cell_CommitHandler,
    sizeof(struct State)
  },
  {0},
};

// Every LP in the PCS model has the same type.
tw_lptype* pcs_type_map(tw_lpid global_id) {
  return &mylps[0];
}

tw_lpid pcs_init_map(unsigned chare, tw_lpid local_id) {
	// Things to know about the PCS grid mapping:
	// 1. The grid is always square
	// 2. The dimensions are always powers of two
	// 3. What used to be KPs are now LPChares. The particular PE doesn't matter
	// 4. THE UNDERLYING ACTUAL MAPPING DOES NOT MATTER
	//    As long as all the routing-map functions are consistent for event sending,
	//    the actual "movement" functions do their own thing
        int cell_vp_x = (NUM_CELLS_X / NUM_VP_X);
        int cell_vp_y = (NUM_CELLS_Y / NUM_VP_Y);

        int chare_x = chare % NUM_VP_X;
        int chare_y = chare / NUM_VP_X;
       
        int off_x = chare_x * cell_vp_x;
        int off_y = chare_y * cell_vp_y;

        off_x += (local_id % cell_vp_x);
        off_y += (local_id / cell_vp_x);

	return (tw_lpid) (off_y * NUM_CELLS_X + off_x);
}

int
main(int argc, char **argv)
{
  tw_init(&argc, &argv);

  if( tw_ismaster() )
  {
    printf("Running simulation with following configuration: \n" );
    printf("\n\n");
  }

  int nlp_per_pe = (NUM_CELLS_X * NUM_CELLS_Y) / (tw_nnodes());
  int additional_memory_buffers = 2 * g_tw_mblock * g_tw_gvt_interval;
  g_tw_max_events_buffered = (nlp_per_pe * BIG_N) + additional_memory_buffers + 50000;

  // Total LPs = (NUM_CELLS_X * NUM_CELLS_Y);
  g_num_chares = (NUM_VP_X * NUM_VP_Y);
  g_lps_per_chare = (NUM_CELLS_X * NUM_CELLS_Y) / (NUM_VP_X * NUM_VP_Y);

  g_type_map = pcs_type_map;
  g_init_map = pcs_init_map;
  g_local_map = pcs_local_map;
  g_chare_map = pcs_grid_map;

  /*
   * Some some of the settings.
   */
  if( tw_ismaster() )
    {
      printf("\n\n");
      printf("/**********************************************/\n");
      printf("MOVE CALL MEAN   = %f\n", MOVE_CALL_MEAN);
      printf("NEXT CALL MEAN   = %f\n", NEXT_CALL_MEAN);
      printf("CALL TIME MEAN   = %f\n", CALL_TIME_MEAN);
      printf("NUM CELLS X      = %d\n", NUM_CELLS_X);
      printf("NUM CELLS Y      = %d\n", NUM_CELLS_Y);
      printf("NUM VP X         = %d\n", NUM_VP_X);
      printf("NUM VP Y         = %d\n", NUM_VP_Y);
      printf("NUM LP Chares    = %d\n", g_num_chares);
      printf("NUM lps per Chare= %d\n", g_lps_per_chare);
      printf("/**********************************************/\n");
      printf("\n\n");
      fflush(stdout);
    }

  tw_define_lps(sizeof(struct Msg_Data), 0);

  /*
   * Initialize App Stats Structure
   */
#ifdef CELL_STATS
  TWAppStats.Call_Attempts = 0;
  TWAppStats.Call_Attempts = 0;
  TWAppStats.Channel_Blocks = 0;
  TWAppStats.Busy_Lines = 0;
  TWAppStats.Handoff_Blocks = 0;
  TWAppStats.Portables_In = 0;
  TWAppStats.Portables_Out = 0;
  TWAppStats.Blocking_Probability = 0.0;
#endif

  tw_run();

#ifdef CELL_STATS
  if( tw_ismaster() ) {
    CellStatistics_Compute(&TWAppStats);
    CellStatistics_Print(&TWAppStats);
  }
#endif

  tw_end();

  return 0;
}
