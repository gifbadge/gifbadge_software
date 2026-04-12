/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "sdkconfig.h"
#include "esp_efuse.h"
#include "esp_efuse_custom_table.h"

// md5_digest_table 1cc13c25161e1fe36796db43a91ab759
// This file was generated from the file esp_efuse_custom_table.csv. DO NOT CHANGE THIS FILE MANUALLY.
// If you want to change some fields, you need to change esp_efuse_custom_table.csv file
// then run `efuse_common_table` or `efuse_custom_table` command it will generate this file.
// To show efuse_table run the command 'show_efuse_table'.

static const esp_efuse_desc_t USER_DATA_BOARD[] = {
    {EFUSE_BLK3, 0, 8}, 	 // Board Type,
};

static const esp_efuse_desc_t KEY0_SERIAL[] = {
    {EFUSE_BLK4, 0, 48}, 	 // Board Serial,
};

static const esp_efuse_desc_t KEY1_SERIAL[] = {
    {EFUSE_BLK5, 0, 48}, 	 // Board Serial,
};





const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_BOARD[] = {
    &USER_DATA_BOARD[0],    		// Board Type
    NULL
};

const esp_efuse_desc_t* ESP_EFUSE_KEY0_SERIAL[] = {
    &KEY0_SERIAL[0],    		// Board Serial
    NULL
};

const esp_efuse_desc_t* ESP_EFUSE_KEY1_SERIAL[] = {
    &KEY1_SERIAL[0],    		// Board Serial
    NULL
};
