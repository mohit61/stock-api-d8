<?php
/**
 * @file
 * Stock API module API file.
 */

/**
 * Alter Google Finance query.
 *
 * @param string $path
 *   Google finance host URL, i.e. 'https://finance.google.com/' or
 *   'https://finance.google.ca/'.
 *
 *   You may want to change it sometimes since Finance may return different data
 *   for different domains.
 * @param array $options
 *   Array or URL options, as passed to url().
 * @param string $stock_market
 *   Stock market name, i.e. 'NASDAQ' or 'TSE'.
 * @param string $stock_symbol
 *   Stock symbol.
 *
 * @see stockapi_google_finance_url()
 * @see url()
 */
function hook_stockapi_google_finance_query_alter(&$path, &$options, $stock_market, $stock_symbol) {
  if ($stock_symbol == 'GOOG') {
    $path = 'https://finance.google.com/';

    // Omit stock market because why not, that's why.
    $options['q'] = $stock_symbol;
  }
}

/**
 * React on Stock API data update.
 *
 * Third-party modules may utilize this hook to clear own caches or process
 * own data.
 * Example usages: 1) Use in a custom module to store stocks data in a JSON
 * file so that you can load with JavaScript code on a fully cached site.
 * 2) Comparing and logging values.
 *
 * @param array $symbols
 *   Stock symbols, as returned by stockapi_fetch().
 *
 * @see stockapi_fetch()
 */
function hook_stockapi_post_update(array $symbols) {
  foreach ($symbols as $key => $stock) {
    // Process own data here.
  }
}
