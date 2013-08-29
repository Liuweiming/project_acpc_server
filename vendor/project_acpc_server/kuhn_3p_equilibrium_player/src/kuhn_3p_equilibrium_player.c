/**
 * kuhn_3p_equilibrium_player.c
 * author:      Dustin Morrill (morrill@ualberta.ca)
 * date:        August 21, 2013
 * description: Player for 3-player Kuhn poker that plays according to an
 *              equilibrium component strategy specified by its given six
 *              parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include "CException.h"
void print_and_throw_error(const char const *message)
{
  fprintf(stderr, "ERROR: %s", message);
  Throw(1);
}
#define DEBUG_PRINT(...) \
  if (DEBUG) { \
    printf(__VA_ARGS__); \
    fflush(NULL); \
  }

#include "kuhn_3p_equilibrium_player.h"

bool is_3p_kuhn_poker_game(const Game const *game_def)
{
  assert(game_def);
  return (
      game_def->bettingType == limitBetting &&
      game_def->numRounds == 1 && game_def->maxRaises[0] == 1 &&
      game_def->numSuits == 1 && game_def->numRanks == 4 &&
      game_def->numHoleCards == 1 &&
      game_def->numBoardCards[0] == 0 &&
      game_def->numPlayers == 3
 );
}

double beta(double b11, double b21)
{
  return fmax(b11, b21);
}

size_t sub_family_number(double c11)
{
  for (size_t sub_family_index = 0; sub_family_index < NUM_SUB_FAMILIES - 1; ++sub_family_index)
  {
    if (c11 == SUB_FAMILY_DEFINING_PARAM_VALUES[sub_family_index])
    {
      // c11 is 0 or 1/2
      return sub_family_index + 1;
    }
  }

  if (c11 > SUB_FAMILY_DEFINING_PARAM_VALUES[NUM_SUB_FAMILIES - 2])
  {
    return NUM_SUB_FAMILIES + 1; // Illegal sub-family number
  }

  // c11 is between 0 and 1/2
  return NUM_SUB_FAMILIES;
}

void check_family_1_params(
    kuhn_3p_equilibrium_player_t* kuhn_3p_e_player
)
{
  assert(kuhn_3p_e_player);

  if (kuhn_3p_e_player->params[B21_INDEX] > 1/4.0)
  {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: b21 greater than 1/4\n"
    );
  }
  if (
      kuhn_3p_e_player->params[B11_INDEX] >
      kuhn_3p_e_player->params[B21_INDEX]
  )
  {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: b11 greater than b21\n"
    );
  }
  if (
      kuhn_3p_e_player->params[B32_INDEX] > (
          (
              2 + 3 * kuhn_3p_e_player->params[B11_INDEX] +
              4 * kuhn_3p_e_player->params[B21_INDEX]
          ) /
          4.0
      )
  )
  {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: b32 too large for any sub-family 1 "
        "equilibrium\n"
    );
  }
  if (
      kuhn_3p_e_player->params[C33_INDEX] < (
          (1/2.0) - kuhn_3p_e_player->params[B32_INDEX]
      )
  )
  {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: c33 too small for any sub-family 1 "
        "equilibrium\n"
    );
  }
  if (
      kuhn_3p_e_player->params[C33_INDEX] > (
          (1/2.0) - kuhn_3p_e_player->params[B32_INDEX] + (
              3 * kuhn_3p_e_player->params[B11_INDEX] +
              4 * kuhn_3p_e_player->params[B21_INDEX]
          ) / 4.0
      )
  )
  {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: c33 too large for any sub-family 1 "
        "equilibrium\n"
    );
  }

  kuhn_3p_e_player->params[B23_INDEX] = 0.0;
  kuhn_3p_e_player->params[B33_INDEX] = (
      1 +
      kuhn_3p_e_player->params[B11_INDEX] +
      2 * kuhn_3p_e_player->params[B21_INDEX]
  ) / 2.0;
  kuhn_3p_e_player->params[B41_INDEX] = (
      2 * kuhn_3p_e_player->params[B11_INDEX] +
      2 * kuhn_3p_e_player->params[B21_INDEX]
  );
  kuhn_3p_e_player->params[C21_INDEX] = 1/2.0;
  return;
}

void check_params(kuhn_3p_equilibrium_player_t* kuhn_3p_e_player)
{
  assert(kuhn_3p_e_player);

  switch (
      sub_family_number(
          kuhn_3p_e_player->params[SUB_FAMILY_DEFINING_PARAM_INDEX]
      )
  )
  {
  case 1:
    check_family_1_params(kuhn_3p_e_player);
    break;
  case 2:
//    check_family_2_params(kuhn_3p_e_player);
    break;
  case 3:
//    check_family_3_params(kuhn_3p_e_player);
    break;
  default:
    print_and_throw_error(
        "kuhn_3p_equilibrium_player: c11 parameter outside of range for any "
        "equilibrium sub-family\n"
    );
    return;
  }

  for (size_t i = 0; i < NUM_PARAMS; ++i) {
    if (
        kuhn_3p_e_player->params[i] < 0.0 ||
        kuhn_3p_e_player->params[i] > 1.0
    ) {
      print_and_throw_error(
         "kuhn_3p_equilibrium_player: strategy parameters must be in [0,1]\n"
      );
    }
  }
  return;
}

void action_probs_p0(
    uint8_t card_rank,
    const State const* state,
    double* probs
) {
  assert(probs);
  assert(state);

  size_t situation_index = 0;

  if (0 == state->numActions[0]) { // Situation 1
    DEBUG_PRINT("action_probs_p0: situation 1\n");

    probs[a_fold] = 0.0;
    probs[a_call] = 1.0 - A[card_rank - JACK_RANK][0];
    probs[a_raise] = A[card_rank - JACK_RANK][0];
    return;
  } else if (a_call == state->action[0][1].type) { // Situation 2
    situation_index = 1;
  } else if (a_fold == state->action[0][2].type) { // Situation 3
    situation_index = 2;
  } else { // Situation 4
    situation_index = 3;
  }

  DEBUG_PRINT("action_probs_p0: situation %u\n", situation_index);

  probs[a_fold] = 1.0 - A[card_rank - JACK_RANK][situation_index];
  probs[a_call] = A[card_rank - JACK_RANK][situation_index];
  probs[a_raise] = 0.0;

  return;
}

void action_probs_p1(
    const double const* params,
    uint8_t card_rank,
    const State const* state,
    double* probs
) {
  assert(params);
  assert(probs);
  assert(state);

  double param;

  // Situation 1 or 2
  if (1 == state->numActions[0]) {
    if (a_call == state->action[0][0].type) { // Situation 1

      DEBUG_PRINT("action_probs_p1: situation 1\n");

      if (JACK_RANK == card_rank) {
        param = params[B11_INDEX];
      } else if (QUEEN_RANK == card_rank) {
        param = params[B21_INDEX];
      } else if (KING_RANK == card_rank) {
        param = B31;
      } else {
        param = params[B41_INDEX];
      }
      probs[a_fold] = 0.0;
      probs[a_call] = 1.0 - param;
      probs[a_raise] = param;
      return;
    } else { // Situation 2

      DEBUG_PRINT("action_probs_p1: situation 2\n");

      if (JACK_RANK == card_rank) {
        param = B12;
      } else if (QUEEN_RANK == card_rank) {
        param = B22;
      } else if (KING_RANK == card_rank) {
        param = params[B32_INDEX];
      } else {
        param = B42;
      }
    }
  } else { // Situation 3 or 4
    if (a_fold == state->action[0][3].type) { // Situation 3

      DEBUG_PRINT("action_probs_p1: situation 3\n");

      if (JACK_RANK == card_rank) {
        param = B13;
      } else if (QUEEN_RANK == card_rank) {
        param = params[B23_INDEX];
      } else if (KING_RANK == card_rank) {
        param = params[B33_INDEX];
      } else {
        param = B43;
      }
    } else { // Situation 4

      DEBUG_PRINT("action_probs_p1: situation 4\n");

      if (JACK_RANK == card_rank) {
        param = B14;
      } else if (QUEEN_RANK == card_rank) {
        param = B24;
      } else if (KING_RANK == card_rank) {
        param = B34;
      } else {
        param = B44;
      }
    }
  }

  probs[a_fold] = 1.0 - param;
  probs[a_call] = param;
  probs[a_raise] = 0.0;

  return;
}

void action_probs_p2(
    const double const* params,
    uint8_t card_rank,
    const State const* state,
    double* probs
)
{
  assert(params);
  assert(probs);
  assert(state);

  double param;

  // Situation 1
  if (a_call == state->action[0][0].type) {
    if (a_call == state->action[0][1].type) { // Situation 1

      DEBUG_PRINT("action_probs_p2: situation 1\n");

      if (JACK_RANK == card_rank) {
        param = params[C11_INDEX];
      } else if (QUEEN_RANK == card_rank) {
        param = params[C21_INDEX];
      } else if (KING_RANK == card_rank) {
        param = C31;
      } else {
        param = C4[0];
      }
      probs[a_fold] = 0.0;
      probs[a_call] = 1.0 - param;
      probs[a_raise] = param;
      return;
    } else { // Situation 2

      DEBUG_PRINT("action_probs_p2: situation 2\n");

      if (JACK_RANK == card_rank) {
        param = C12;
      } else if (QUEEN_RANK == card_rank) {
        param = C22;
      } else if (KING_RANK == card_rank) {
        param = C32;
      } else {
        param = C4[1];
      }
    }
  } else { // Situation 3 or 4
    if (a_fold == state->action[0][1].type) { // Situation 3

      DEBUG_PRINT("action_probs_p2: situation 3\n");

      if (JACK_RANK == card_rank) {
        param = C13;
      } else if (QUEEN_RANK == card_rank) {
        param = C23;
      } else if (KING_RANK == card_rank) {
        param = params[C33_INDEX];
      } else {
        param = C4[2];
      }
    } else { // Situation 4

      DEBUG_PRINT("action_probs_p2: situation 4\n");

      if (JACK_RANK == card_rank) {
        param = C14;
      } else if (QUEEN_RANK == card_rank) {
        param = C24;
      } else if (KING_RANK == card_rank) {
        param = params[C34_INDEX];
      } else {
        param = C4[3];
      }
    }
  }

  probs[a_fold] = 1.0 - param;
  probs[a_call] = param;
  probs[a_raise] = 0.0;

  return;
}

/*********************************/
/****** Player Interface *********/
/*********************************/

/* create any private space needed for future calls
   game_def is a private copy allocated by the dealer for the player's use */
kuhn_3p_equilibrium_player_t init_private_info(
    const Game const* game_def,
    const double const* params,
    uint32_t seed
)
{
  assert(game_def);
  assert(params);

  kuhn_3p_equilibrium_player_t kuhn_3p_e_player;

  /* This player cannot be used outside of Kuhn poker */
  if(!is_3p_kuhn_poker_game(game_def)) {
    print_and_throw_error(
        "kuhn_3p_equilibrium_player used in non-Kuhn game\n"
    );
    return kuhn_3p_e_player;
  }

  kuhn_3p_e_player.game_def = game_def;

  memset(
      kuhn_3p_e_player.params,
      0,
      NUM_PARAMS * sizeof(*kuhn_3p_e_player.params)
  );
  for (size_t i = 0; i < NUM_PARAMS; ++i) {
    kuhn_3p_e_player.params[i] = params[i];
  }

  kuhn_3p_e_player.seed = seed;

  CEXCEPTION_T e = 0;
  Try {
    check_params(&kuhn_3p_e_player);
  } Catch(e) {
    Throw(e);
  }

  /* Create our random number generators */
  init_genrand(&kuhn_3p_e_player.get_action_rng, kuhn_3p_e_player.seed);

  return kuhn_3p_e_player;
}

/* return a (valid!) action at the information state described by view */
Action action(
    kuhn_3p_equilibrium_player_t* player,
    MatchState view
)
{
  assert(player);

  double probs[NUM_ACTION_TYPES];
  memset(probs, 0, NUM_ACTION_TYPES * sizeof(*probs));

  ////////// @todo
  DEBUG_PRINT("action: \n================\n");
  char ms_string[255];
  printMatchState(player->game_def, &view, 255, ms_string);
  DEBUG_PRINT("ms: %s\n", ms_string);
//////

  action_probs(player, view, probs);

  ////////@todo
  for (int i = 0; i < 3; ++i) {
    DEBUG_PRINT("action %d: %lf\n", i, probs[i]);
  }
  ///////////


  double r = genrand_real2(&player->get_action_rng);
  enum ActionType i = a_fold;
  for(; i < NUM_ACTION_TYPES; i++) {
    if(r <= probs[i]) {
      break;
    }
    r -= probs[i];
  }

  Action a = {a.type = i, a.size = 0};

  return a;
}

/* Returns a distribution of actions in action_triple
   This is an extra function, and does _NOT_ need to be implemented
   to be considered a valid player.h interface. */
void action_probs(
    const kuhn_3p_equilibrium_player_t const* player,
    MatchState view,
    double* probs
)
{
  assert(player);
  assert(probs);

  uint8_t card_rank = rankOfCard(view.state.holeCards[view.viewingPlayer][0]);

  DEBUG_PRINT("action_probs: card rank: %u, "
      "viewingPlayer: %u\n", card_rank, view.viewingPlayer);

  if (0 == view.viewingPlayer) {
    action_probs_p0(card_rank, &view.state, probs);
  } else if (1 == view.viewingPlayer) {
    action_probs_p1(player->params, card_rank, &view.state, probs);
  } else {
    action_probs_p2(player->params, card_rank, &view.state, probs);
  }
  return;
}
