<?php 

/**
 * @file
 * Contains \Drupal\stockapi\Controller\StockApiController.
 */

namespace Drupal\stockapi\Controller;

use Drupal\Core\Controller\ControllerBase;

/**
 * StockApi Controller for the stockapi module.
 */
class StockApiController extends ControllerBase {

  public function stockapi_page() {

    $header = [
      'symbol' => [
        'data' => t('Symbol'),
        'field' => 's.symbol',
        'class' => [
          'left'
          ],
      ],
      'name' => [
        'data' => t('Name'),
        'field' => 's.name',
        'class' => [
          'left'
          ],
      ],
      'last_trade_price_only' => [
        'data' => t('Last'),
        'field' => 's.last_trade_price_only',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'chg' => [
        'data' => t('Change'),
        'field' => 's.chg',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'pct_chg' => [
        'data' => t('%'),
        'field' => 's.pct_chg',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'open' => [
        'data' => t('Updated'),
        'field' => 's.open',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'volume' => [
        'data' => t('Volume'),
        'field' => 's.volume',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'last_trade_date' => [
        'data' => t('Date'),
        'field' => 's.last_trade_date',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
      'last_trade_time' => [
        'data' => t('Time'),
        'field' => 's.last_trade_time',
        'class' => [RESPONSIVE_PRIORITY_MEDIUM],
      ],
    ];

    $query = db_select('stockapi', 's')->extend('Drupal\Core\Database\Query\PagerSelectExtender')->extend('Drupal\Core\Database\Query\TableSortExtender');
    $result = $query->fields('s', [
      'symbol',
      'name',
      'last_trade_price_only',
      'chg',
      'pct_chg',
      'open',
      'volume',
      'last_trade_date',
      'last_trade_time',
      'exchange',
    ])
      ->limit(50)
      ->orderByHeader($header)
      ->execute();

    $rows = [];

    while ($data = $result->fetchObject()) {
      $rows[] = [
        stockapi_format_symbol($data->symbol, $data->name, $data->exchange),
        $data->name,
        stockapi_format_align(stockapi_format_decimals($data->last_trade_price_only)),
        stockapi_format_align(stockapi_format_change(stockapi_format_decimals($data->chg))),
        stockapi_format_align(stockapi_format_change(stockapi_format_decimals($data->pct_chg))),
        stockapi_format_align(stockapi_format_decimals($data->open)),
        stockapi_format_align($data->volume),
        $data->last_trade_date,
        $data->last_trade_time,
      ];
    }

    $output = [
      'table' => [
        '#type' => 'table',
        '#header' => $header,
        '#rows' => $rows,
        '#attributes' => [
          'id' => 'stockapi-table'
        ],
        '#sticky' => TRUE,
        '#caption' => t('All available stock quotes.'),
        '#colgroups' => [],
        '#empty' => t('No stock quotes available.')
      ],
      'pager' => [
        '#type' => 'pager',
      ],
      '#attached' => [
        'library' => 'stockapi/stockapi',
      ],
    ];

    return $output;
  }

}
