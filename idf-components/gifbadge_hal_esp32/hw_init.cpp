/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "hal/board.h"
static const char *TAG = "HW_INIT";

static Boards::Board *global_board;

#include "esp_efuse_custom_table.h"
#include "boards/boards.h"
#include "boards/mini/v0.h"
#include "boards/mini/v0_1.h"
#include "boards/full/v0_2.h"
#include "boards/full/v0_4.h"
#include "boards/full/v0_6.h"
#include "boards/mini/v0_3.h"
#include "log.h"

Boards::Board *get_board() {
//    return new board_v0();
  uint8_t board;
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
  if (!global_board) {
      LOGI(TAG, "Board %u", board);

      switch (board) {
      case BOARD_1_28_V0:
        global_board = new Boards::esp32::s3::mini::v0();
        break;
      case BOARD_2_1_V0_2:
        global_board = new Boards::esp32::s3::full::v0_2();
        break;
      case BOARD_1_28_V0_1:
        global_board = new Boards::esp32::s3::mini::v0_1();
        break;
      case BOARD_2_1_V0_4:
        global_board = new Boards::esp32::s3::full::v0_4();
        break;
      case BOARD_1_28_V0_3:
        global_board = new Boards::esp32::s3::mini::v0_3();
        break;
      case BOARD_2_1_V0_6:
      case BOARD_2_1_V0_7:
          global_board = new Boards::esp32::s3::full::v0_6();
        break;
      default:
        return nullptr;
    }
    LOGI(TAG, "Board %s", global_board->Name());
  }
    return global_board;
}
