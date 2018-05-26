<?php

/**
 * @file
 * A wrapper API for the retreiving stock quotes from Alpha Vantage Realtime and historical equity data.
 */

use Drupal\Core\Url;

define("stockapi_symbol", "0");
define("stockapi_trade_price_only", "1");
define("stockapi_chg", "2");
define("stockapi_pct_chg", "3");
define("stockapi_updated", "4");
define("stockapi_previous_close", "5");
define("stockapi_days_range", "6");
define("stockapi_52_week_range", "7");
define("stockapi_open", "8");
define("stockapi_volume", "9");
define("stockapi_avg_daily_volume", "10");
define("stockapi_market_cap", "11");
define("stockapi_trade_date", "12"); //string
define("stockapi_last_trade_date", "13"); //string
define("stockapi_pe_ratio", "14");
define("stockapi_float_shares", "15");
define("stockapi_chg_and_pct_chg", "16");
define("stockapi_last_trade_time", "17");
define("stockapi_exchange", "18");
define("stockapi_name", "19");
define("stockapi_vol_avg_for_postprocessing", "999");

/**
 * Implement a HTTP request to Alpha Vantage Realtime and historical equity data to retrieve stock quotes.
 *
 * @param $symbols array
 *   An array of ticker symbols to send to Alpha Vantage. There is not limit on the
 *   the request into batches.
 *
 * @return
 *   An array of stocks with stock information from Alpha Vantage. False on failure.
 */
function stockapi_fetch($symbols) {
  // Step 1: Split up larger arrays.
  $stock_lookup_failure = array();
  $mkt = '';
  $stock_results = array();
  $l = 199;
  if (count($symbols) <= $l) {
    $stock_array = $symbols;

    foreach($stock_array as $stock_symbol) {
      $stock_test = '';
      $stock_name = '';
      $result = db_query("SELECT exchange FROM {stockapi} WHERE symbol = :symbol", array(':symbol' => $stock_symbol))->fetchField();
      $resultName = db_query("SELECT name FROM {stockapi} WHERE symbol = :symbol", array(':symbol' => $stock_symbol))->fetchField();
      if (empty($result)) {
        $stock_test = 'TSE';
      }
      else {
        $stock_test = $result;
      }
      if (empty($resultName)) {
        $stock_name = $stock_test . ':' . $stock_symbol;
      }
      else {
        $stock_name = $resultName;
      }
      switch ($stock_test) {
        case 'NYSE':
          $mkt = 'NYSE';
        break;
        case 'TSE':
          $mkt = 'TSE';
        break;
        case 'Toronto Stock Exchange':
          $mkt = 'TSE';
        break;
        case 'New York Stock Exchange':
          $mkt = 'NYSE';
        break;
        case 'NASDAQ':
          $mkt = 'NASDAQ';
        break;
        case '':
          $mkt = 'TSE';
        break;
        case NULL:
          $mkt = 'TSE';
        break;
        default:
          $mkt = 'TSE';
        break;
      }

      if (\Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey') == 'demo' ||
          \Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey') == '') {
        \Drupal::configFactory()->getEditable('stockapi.settings')->set('stockapi_vantage_apikey', 'demo')->save();
        //$stock_symbol = 'AAPL'; //demo api key only works for a few demo stocks like AAPL or MSFT.
        $mkt = 'NYSE';
        $warning_key = t('Using demo key, please register for a free api key' .
         ' at https://www.alphavantage.co/support/#api-key then set the key ' .
         ' at /admin/config/services/stockapi and press save.');
        drupal_set_message($warning_key, $type = 'warning', $repeat = FALSE);
      }
      $sym = $stock_symbol;
      $series = 'TIME_SERIES_INTRADAY';
      $series_m = 'TIME_SERIES_MONTHLY';
      $series_d = 'TIME_SERIES_DAILY';
      $urld = stockapi_alpha_vantage_json_url($mkt, $sym, $series_d, NULL);
      $interval = '15min';
      $url = stockapi_alpha_vantage_json_url($mkt, $sym, $series, $interval);
      // Load JSON.
      $json_intraday = _file_get_contents($url);
      $intraday_array = json_decode($json_intraday, TRUE);
      $daily_array = array();
      $json_daily = NULL;
      if (isset($intraday_array['Information'])) {
        $info = $intraday_array['Information'];
        \Drupal::logger('stockapi_info')->warning('To use this api provider you must login and' .
           ' register for an api key at ' .
           'https://www.alphavantage.co/support/#api-key <pre>@print_r</pre>', array('@print_r' => print_r($intraday_array['Information'], TRUE)
          ));
      }
      if (isset($intraday_array['Error Message'])) {
        // Load JSON.
        try {
          $json_daily = _file_get_contents($urld);
          $daily_array = json_decode($json_daily, TRUE);
          if (isset($daily_array['Error Message'])) {
            \Drupal::logger('stockapi_info')->notice('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
                print_r($daily_array['Error Message'], TRUE),
                '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series_d,
                '@print_url' => $urld,
              ));
            $json_daily = NULL; //Use this like a flag.
            throw new Exception("no stock data for sym: $sym for mkt: $mkt in" .
            " $series_d " . $daily_array['Error Message']);
          }
          //grab daily
        } catch (Exception $e) {
          if ($mkt == 'NYSE') {
            $mkt = 'TSE';
          } else {
            $mkt = 'NYSE';
          }
          \Drupal::logger('stockapi_info')->warning('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
              print_r($intraday_array['Error Message'], TRUE),
              '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series,
              '@print_url' => $url,
            ));
          $url = stockapi_alpha_vantage_json_url($mkt, $sym, $series, $interval);
          $json_intraday = _file_get_contents($url);
          $intraday_array = json_decode($json_intraday, TRUE);
          if (isset($intraday_array['Error Message'])) {
            $mkt = 'NASDAQ';
            $url = stockapi_alpha_vantage_json_url($mkt, $sym, $series, $interval);
            $json_intraday = _file_get_contents($url);
            $intraday_array = json_decode($json_intraday, TRUE);
            if (isset($intraday_array['Error Message'])) {
              \Drupal::logger('stockapi_info')->warning('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
                  print_r($intraday_array['Error Message'], TRUE),
                  '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series,
                  '@print_url' => $url,
                ));
              break; //bail on this
            }
          }
        }
      }
      $urlm = stockapi_alpha_vantage_json_url($mkt, $sym, $series_m, NULL);
      $json_monthly = _file_get_contents($urlm);
      $monthly_array = json_decode($json_monthly, TRUE);
      if (isset($monthly_array['Error Message'])) {
        \Drupal::logger('stockapi_info')->warning('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
            print_r($monthly_array['Error Message'], TRUE),
            '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series_m,
            '@print_url' => $urlm,
          ));
        break; //bail on this
      }
      $m_key = 'Monthly Time Series';
      $m_key_adj = 'Monthly Adjusted Time Series';
      if (!isset($monthly_array[$m_key])) {
        $m_key = $m_key_adj;
        if (!isset($monthly_array[$m_key])) {
          \Drupal::logger('stockapi_info')->warning('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
              print_r('Undefined index: Monthly Adjusted Time Series: monthly_array=' . $monthly_array, TRUE),
              '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series_m,
              '@print_url' => $urlm,
            ));
          continue;
        }
      }
      $m_count = 0;
      $m_total = count($monthly_array[$m_key]);
      $m_avg_vol = 0;
      $m_previous_close = 0;
      $m_52_low = 0;
      $m_52_high = 0;
      $m_cum_vol = 0;
      $m_series_array = $monthly_array[$m_key];
      if (isset($m_series_array)) {
        foreach($m_series_array as $key => $monthly_trading) {
          $m_seg_vol = $monthly_trading['5. volume'];
          if ($m_cum_vol == 0) {
            $m_cum_vol = round($m_seg_vol);
          } else {
            $m_cum_vol = $m_cum_vol + round($m_seg_vol);
          }
          $m_seg_high = $monthly_trading['2. high'];
          $m_seg_low = $monthly_trading['3. low'];
          if ($m_count == 0) {
            // Premier.
            $m_cum_vol = 0;
            $m_seg_high = 0;
            $m_seg_low = 0;
            // Skip the first one, because it likely an incomplete month.
            // except close, grab the latest close. (first)
            $m_previous_close = $monthly_trading['4. close'];
          }
          //if ($m_count == $m_total -1) {
          //  // Dernier.
          //}
          if ($m_seg_low <= $m_52_low) {
            $m_52_low = $m_seg_low;
          } else if ($m_52_low == 0) {
            $m_52_low = $m_seg_low;
          }
          if ($m_seg_high >= $m_52_high) {
            $m_52_high = $m_seg_high;
          }

          $m_count++;
          // Do 13 instead of 12, we skip the first month.
          if ($m_count == 13) {
            // 1 year.
            break;
          }
        }
      }
      // The NYSE and NASDAQ average about 252 trading days a year.
      $m_avg_vol = round(floatval($m_cum_vol/252));

    /*watchdog('stockapi_info', '<pre>@print_debug - @print_url - @print_r</pre>',
        array('@print_r' =>
          print_r('test', TRUE),
          '@print_debug' => $m_count . ' : ' . $sym . " m_cum_vol=$m_cum_vol m_avg_vol=$m_avg_vol m_seg_vol=$m_seg_vol series",
          '@print_url' => $urlm,
        ), WATCHDOG_WARNING
      );*/
//      return FALSE;
      /*
       * JSON looks like this:
       *
      Array
      (
        [Meta Data] => Array
            (
                [1. Information] => Intraday (15min) prices and volumes
                [2. Symbol] => TSE:XYZ
                [3. Last Refreshed] => 2018-03-23 16:00:00
                [4. Interval] => 15min
                [5. Output Size] => Compact
                [6. Time Zone] => US/Eastern
            )

        [Time Series (15min)] => Array
            (
                [2018-03-23 16:00:00] => Array
                    (
                        [1. open] => 1.3000
                        [2. high] => 1.3000
                        [3. low] => 1.3000
                        [4. close] => 1.3000
                        [5. volume] => 17400
                    )

                [2018-03-23 15:55:00] => Array
                    (
                        [1. open] => 1.3100
                        [2. high] => 1.3100
      */
      $count5s = 0; //Count for the foreach loop 5 min series.
      $countDs = 0; //Count for the foreach loop 5 min series.
      $totalNum5s = 0;
      if (isset($intraday_array['Time Series (15min)'])) {
        $totalNum5s = count($intraday_array['Time Series (15min)']);
      }
      $last_updated = 0;
      $last_trade_date = 0;
      $last_trade_time = 0;
      $last_refreshed = 0;
      $localtime_assoc = 0;
      $timezone_correct = TRUE;
      try {
        date_default_timezone_set('EST');
      } catch (Exception $e) {
        $timezone_correct = FALSE;
        \Drupal::logger('stockapi_info')->warning('<pre>@print_r</pre>', array('@print_r' =>
          print_r(t('date_default_timezone_set(\'EST\') failed.'), TRUE)
          ));
      }
      if (isset($intraday_array['Meta Data']['3. Last Refreshed'])) {
        $last_refreshed = $intraday_array['Meta Data']['3. Last Refreshed'];
        $last_refreshed = strtotime($last_refreshed);
      } else {
        $last_refreshed = time();
      }
      if ($timezone_correct) {
        try {
          $localtime_assoc = localtime($last_refreshed, true);
          $min = $localtime_assoc['tm_min'];
          if (strlen($min) == 1) {
            $min = '0' . $min;
          }
          $last_trade_time = $localtime_assoc['tm_hour'] . 'h' . $min;
        } catch (Exception $e) {
          $last_trade_time = date('H\hi', $last_refreshed);
        }
      } else {
        $last_trade_time = date('H\hi', $last_refreshed);
      }
      $trade_date = date('M j', $last_refreshed);
      //Initialize more variables
      $dernier = FALSE;
      $date_of_trade = 0;
      $todays_open = 0;
      $current_date_of_trade = 0;
      $previous_day_of_trade = 0;
      $latest_price = 0;
      $totalNumDs = 0;
      $latest_segment_volume = 0;
      $todays_volume = 0;
      $todays_low = 0;
      $todays_high = 0;
      $segment_low = 0;
      $segment_high = 0;
      $previous_close = 0;
      $pct_chg = 0;
      $todays_chg = 0;
      $flag_for_previous = TRUE;
      if (!empty($json_daily)) {
        if (!isset($daily_array['Time Series (Daily)'])) {
          \Drupal::logger('stockapi_info')->warning('<pre>Time Series (Daily) - @print_debug - @print_url - json response: @print_json_daily</pre>', array('@print_debug' =>  $sym,
                    '@print_url' => $url,
                    '@print_json_daily' => $json_daily,
            ));
          //continue to next stock_symbol
          try {
            \Drupal::logger('stockapi_info')->notice('Sleep 20 seconds processing symbol:<pre>@print_sym</pre>', array('@print_sym' =>  $sym));
            sleep(20); //wait 20 seconds, try again.
            $json_daily = _file_get_contents($urld);
            $daily_array = json_decode($json_daily, TRUE);
            if (isset($daily_array['Error Message'])) {
              \Drupal::logger('stockapi_info')->notice('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
                  print_r($daily_array['Error Message'], TRUE),
                  '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series_d,
                  '@print_url' => $urld,
                ));
              $json_daily = NULL; //Use this like a flag.
              throw new Exception("no stock data for sym: $sym for mkt: $mkt in" .
              " $series_d " . $daily_array['Error Message']);
            }
            if (!isset($daily_array['Time Series (Daily)'])) {
              throw new Exception("no stock data for sym: $sym for mkt: $mkt in" .
              " $series_d " . $daily_array['Error Message']);
            }
          } catch (Exception $e) {
            \Drupal::logger('stockapi_info')->notice('<pre>@print_debug</pre>', array('@print_debug' =>  $e->getMessage()));
            continue;
         }
          //bust out.
          //break;
        }
        $totalNumDs = count($daily_array['Time Series (Daily)']);
        foreach($daily_array['Time Series (Daily)'] as $key => $daily_summary) {
          $day_volume = $daily_summary['5. volume'];
          $date_of_trade = substr($key, 0, strpos($key, ' '));
          if ($countDs == 0) {
            $current_date_of_trade = $date_of_trade;
            $todays_open = $daily_summary['1. open'];
            $todays_high = $daily_summary['2. high'];
            $todays_low = $daily_summary['3. low'];
            $last_updated = strtotime($current_date_of_trade);
            $latest_price = $daily_summary['4. close'];
            $todays_volume = $daily_summary['5. volume'];
            // premier.
          } else if ($countDs == $totalNumDs -1) {
            // dernier.
            $dernier = TRUE;
          }
          if ($todays_open == 0 && $daily_summary['1. open'] != 0 ) {
            $todays_open = $daily_summary['1. open'];
          }
          if ($todays_high == 0 && $daily_summary['2. high'] != 0 ) {
            $todays_high = $daily_summary['2. high'];
          }
          if ($todays_low == 0 && $daily_summary['3. low'] != 0 ) {
            $todays_low = $daily_summary['3. low'];
          }
          if ($latest_price == 0 && $daily_summary['4. close'] != 0 ) {
            $latest_price = $daily_summary['4. close'];
          }
          if ($todays_volume == 0 && $daily_summary['5. volume'] != 0 ) {
            $todays_volume = $daily_summary['5. volume'];
          }
          if ($current_date_of_trade == $date_of_trade) {
            // clean up later
            $test = NULL;
          } else {
            if ($flag_for_previous) {
              $flag_for_previous = FALSE;
              $previous_close = $daily_summary['4. close'];
            }
          }
          /* Debug.
          if ($dernier) {
            watchdog('stockapi_info', '<pre>@print_debug - @print_url - @print_r</pre>',
              array('@print_r' =>
                print_r('test', TRUE),
                '@print_debug' => $countDs . ' : ' . $sym . " day_volume=$day_volume todays_volume=$todays_volume ",
                '@print_url' => $url,
              ), WATCHDOG_WARNING
            );
          }*/
          $countDs++;
        }
      } else {
        if (!isset($intraday_array['Time Series (15min)'])) {
          \Drupal::logger('stockapi_info')->warning('<pre>Time Series (15min) intraday - @print_debug - @print_url - json response: @print_json_intraday</pre>', array('@print_debug' =>  $sym,
                    '@print_url' => $url,
                    '@print_json_intraday' => $json_intraday,
            ));
          //continue to next stock_symbol
          try {
            \Drupal::logger('stockapi_info')->notice('Sleep 15 seconds processing symbol:<pre>@print_sym</pre>', array('@print_sym' =>  $sym));
            sleep(20); //wait 20 seconds, try again.
            $json_intraday = _file_get_contents($url);
            $intraday_array = json_decode($json_intraday, TRUE);
            if (isset($intraday_array['Error Message'])) {
              \Drupal::logger('stockapi_info')->notice('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
                  print_r($intraday_array['Error Message'], TRUE),
                  '@print_debug' => $mkt . ' : ' . $sym . ' series: ' . $series,
                  '@print_url' => $url,
                ));
              $json_intraday = NULL; //Use this like a flag.
              throw new Exception("no stock data for sym: $sym for mkt: $mkt in" .
              " $series " . $intraday_array['Error Message']);
            }
            if (!isset($intraday_array['Time Series (15min)'])) {
              throw new Exception("no stock data for sym: $sym for mkt: $mkt in" .
              " $series " . $intraday_array['Error Message']);
            }
          } catch (Exception $e) {
            \Drupal::logger('stockapi_info')->notice('<pre>@print_debug</pre>', array('@print_debug' =>  $e->getMessage()));
            continue;
          }
          //bust out.
          break;
        }
        foreach($intraday_array['Time Series (15min)'] as $key => $five_minutes_trading) {
          $segment_volume = $five_minutes_trading['5. volume'];
          $date_of_trade = substr($key, 0, strpos($key, ' '));
          if ($count5s == 0) {
            $current_date_of_trade = $date_of_trade;
            $todays_high = $five_minutes_trading['2. high'];
            $todays_low = $five_minutes_trading['3. low'];
            $last_updated = strtotime($current_date_of_trade);
            $latest_price = $five_minutes_trading['4. close'];
            $latest_segment_volume = $five_minutes_trading['5. volume'];
            // premier.
          } else if ($count5s == $totalNum5s -1) {
            // dernier.
            $dernier = TRUE;
          }
          if ($current_date_of_trade == $date_of_trade) {
            $segment_high = $five_minutes_trading['2. high'];
            $segment_low = $five_minutes_trading['3. low'];
            if ($count5s > 0) {
              $latest_segment_volume = $five_minutes_trading['5. volume'];
            }
            $todays_volume = $todays_volume + $latest_segment_volume;
            $todays_open = $five_minutes_trading['1. open'];
            if ($segment_low < $todays_low) {
              $todays_low = $segment_low;
            }
            if ($segment_high > $todays_high) {
              $todays_high = $segment_high;
            }
          } else {
            if ($flag_for_previous) {
              $flag_for_previous = FALSE;
              $previous_close = $five_minutes_trading['4. close'];
            }
          }
          $count5s++;
        }
      }
      if ($previous_close == 0 && $m_previous_close != 0) {
        $previous_close = $m_previous_close;
      } else if ($previous_close == 0) {
        // Debug.
        if ($dernier) {
          \Drupal::logger('stockapi_info')->warning('<pre>@print_debug - @print_url - @print_r</pre>', array('@print_r' =>
              print_r('test', TRUE),
              '@print_debug' => 'previous_close =0  : ' . $sym . " todays_volume=$todays_volume ",
              '@print_url' => $url . ' urlm= ' . $urlm . ' urld=' . $urld,
            ));
        }
        break; //bail on this
      }
      $stock_results[$stock_symbol][stockapi_avg_daily_volume] = round($m_avg_vol);
      $stock_results[$stock_symbol][stockapi_trade_date] = $trade_date;
      $stock_results[$stock_symbol][stockapi_last_trade_date] = $trade_date;
      $stock_results[$stock_symbol][stockapi_last_trade_time] = $last_trade_time;
      $stock_results[$stock_symbol][stockapi_updated] = time();
      $stock_results[$stock_symbol][stockapi_name] = $stock_name;
      $stock_results[$stock_symbol][stockapi_exchange] = $mkt;

      //return FALSE;
      //define("stockapi_pct_chg", "3");
      $todays_chg = 0.0;
      $pct_chg = 0;
      $rnd2deci = \Drupal::config('stockapi.settings')->get('stockapi_decimals');
      $todays_chg = (floatval($latest_price) - floatval($previous_close));
      $todays_chg = round($todays_chg, $rnd2deci);
      $todays_len = strlen($todays_chg);
      $todays_format = '';
      if (strpos($todays_chg, '.') == ($todays_len-2)) {
        $todays_format = '0';
      } else if ($todays_len == 1) {
        $todays_format = '.00';
      }
      if ($previous_close == 0) {
        // prevent division by zero in case above code fails.
        $previous_close = $latest_price;
      }
      $tmp_pct = round(floatval($todays_chg * -1 / $previous_close * 100), $rnd2deci);
      $tmp_len = strlen($tmp_pct);
      $tmp_format = '';
      if (strpos($tmp_pct, '.') == ($tmp_len-2)) {
        $tmp_format = '0';
      }
      if ($todays_chg >= 0 && $previous_close != 0) {
        $pct_chg = round(floatval($todays_chg / $previous_close * 100), $rnd2deci) . $tmp_format . '%';
      } else if ($previous_close !=0) {
        $tmp_chg = round(floatval($todays_chg * -1), $rnd2deci);
        $pct_chg = '-' . round(floatval($tmp_chg / $previous_close * 100), $rnd2deci) . $tmp_format . '%';
      } else {
        $pct_chg = '0%';
      }
      $chg_and_pct_chg = $todays_chg . $todays_format . ' (' . $pct_chg . ')';
      $stock_results[$stock_symbol][stockapi_trade_price_only] = $latest_price;
      $stock_results[$stock_symbol][stockapi_chg_and_pct_chg] = $chg_and_pct_chg;
      $stock_results[$stock_symbol][stockapi_pct_chg] = $pct_chg;
      $stock_results[$stock_symbol][stockapi_chg] = $todays_chg;
      $stock_results[$stock_symbol][stockapi_volume] = $todays_volume;
      $stock_results[$stock_symbol][stockapi_open] = $todays_open;
      $stock_results[$stock_symbol][stockapi_previous_close] = $previous_close;
      $stock_results[$stock_symbol][stockapi_52_week_range] = round($m_52_low, $rnd2deci) . ' - ' . round($m_52_high, $rnd2deci);
      $stock_results[$stock_symbol][stockapi_days_range] = $todays_low . ' - ' . $todays_high;

      // Fill out empty values, current implementation doesn't have this info.
      //TODO add more info from other sources later.
      $stock_results[$stock_symbol][stockapi_pe_ratio] = 0;
      $stock_results[$stock_symbol][stockapi_float_shares] = 0;
      $stock_results[$stock_symbol][stockapi_market_cap] = 0;
      $stock_results[$stock_symbol][stockapi_symbol] = $stock_symbol;
      //DEBUG ..
      /*
      $test = "$date_of_trade, $current_date_of_trade, $previous_day_of_trade, " .
              "$last_updated, $latest_price, $latest_segment_volume, " .
              "$todays_volume, $todays_high, $todays_low, $previous_close, " .
              "$todays_chg, $chg_and_pct_chg";
      watchdog('stockapi_info', '<pre>@print_r</pre>', array('@print_r' =>
        print_r($test , TRUE)
        ), WATCHDOG_INFO
      ); */
      if (\Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey') == 'demo' && $stock_symbol != 'AAPL' && $stock_symbol != 'MSFT') {
        //Just finished processing stock symbol AAPL, that is enough of a demo.
        break;
      }

      //DEBUG
      /*watchdog('stockapi_info', '<pre>@print_r</pre>', array('@print_r' =>
        print_r($stock_results, TRUE)
        ), WATCHDOG_INFO
      );*/ //DEBUG
    }
    //watchdog('stockapi_results', '<pre>@print_r</pre>', array('@print_r' => print_r( $stock_results, TRUE)));
    //return FALSE; //DEBUGING
    return $stock_results;
  }
  else {
    $batches = array_chunk($symbols, $l);
    $stocks = array();
    foreach ($batches as $key => $batch) {
      $stocks = array_merge(stockapi_fetch($batch), $stocks);
    }
    return $stocks;
  }
}

/**
 * Generate Alpha Vantage - Free JSON api URL.
 *
 * @param string $mkt
 *   Stock market name, i.e. 'NASDAQ' or 'TSE'.
 * @param string $stock_symbol
 *   Stock symbol.
 *
 * @return string
 *   Alpha Vantage API url for given stock market and symbol.
 */
function stockapi_alpha_vantage_json_url($mkt, $stock_symbol, $time_series, $interval) {
  if ($mkt != 'TSE') {
    $mkt = '';
  } else {
    $mkt = $mkt . ':';
  }
  if (strpos($stock_symbol,'.') >= 1) {
    $stock_symbol = str_replace('.','-',$stock_symbol);
  }
  $symbol_key = 'symbol'; //Currently not used, for BATCH_STOCK_QUOTES.
  if (stripos($stock_symbol,',') >= 1) {
    $symbol_key = 'symbols';//For BATCH_STOCK_QUOTES, not yet used.
    $time_series = 'BATCH_STOCK_QUOTES';
  }
  //$time_series options = TIME_SERIES_DAILY_ADJUSTED
  //$time_series options = TIME_SERIES_INTRADAY
  //$time_series options = TIME_SERIES_WEEKLY
  //$time_series options = TIME_SERIES_WEEKLY_ADJUSTED
  //$time_series options = TIME_SERIES_MONTHLY
  //$time_series options = BATCH_STOCK_QUOTES
  //$interval = 1min
  //$interval = 5min
  //$interval = 10min
  //$interval = 15min
  //$interval = 30min
  //$interval = 60min

  $params_array=array();
  if (empty($interval)) {
    $params_array = array(
      'function' => $time_series,
      $symbol_key => $mkt . $stock_symbol,
      'apikey' => \Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey'),
    );
  } else {
    $params_array = array(
      'function' => $time_series,
      $symbol_key => $mkt . $stock_symbol,
      'interval' => $interval,
      'apikey' => \Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey'),
    );
  }
  $host = 'https://www.alphavantage.co/query';
  $options = array(
    'query' => $params_array,
    'external' => TRUE,
    'absolute' => TRUE,
    'https' => TRUE,
  );

  \Drupal::moduleHandler()->alter('stockapi_alpha_vantage_json_url', $host, $options, $mkt, $stock_symbol, $time_series, $interval);

  return Url::fromUri($host, $options)->toString();

}

function _stockapi_money_to_float($money_string) {
  $money_string = trim($money_string);
  $idx_of_scale = strlen($money_string);
  $idx_of_comma = strpos($money_string, ',');
  if (is_numeric($money_string)) {
    $money_float = floatval($money_string);
  }
  else {
    $scale = substr($money_string, -1);
    if ($idx_of_comma >= 0) {
      $money_string = str_replace(',', '', $money_string);
    }
    switch ($scale) {
      case 'M' :
        $money_float = floatval($money_string) * 1000000;
      break;
      case 'm' :
        $money_float = floatval($money_string) * 1000000;
      break;
      case 'B' :
        $money_float = floatval($money_string) * 1000000000;
      break;
      case 'b' :
        $money_float = floatval($money_string) * 1000000000;
      break;
      default:
        $money_float = floatval($money_string);
      break;
    }
  }
  return $money_float;
}

/*
 * Convert scraped key into the db schema key.
 */
function _stockapi_process_key($key) {

  $key = trim($key);
  switch ($key) {
    case 'Range' :
      $key = stockapi_days_range;
    break;
    case '52 week' :
      $key = stockapi_52_week_range;
    break;
    case 'Open' :
      $key = stockapi_open;
    break;
    // needs post processing, comes in as Vol / Avg.
    case 'Vol / Avg.' :
      $key = stockapi_vol_avg_for_postprocessing;
      // Split this later into stockapi_volume and stockapi_avg_daily_volume.
    break;
    case 'Mkt cap' :
      $key = stockapi_market_cap;
    break;
    case 'P/E' :
      $key = stockapi_pe_ratio;
    break;
    case 'Shares' :
      $key = stockapi_float_shares;
    break;
  }
  return $key;
}

/*
 * Convert scraped val into format expected by schema.
 */
function _stockapi_process_val($val) {
  return $val;
}

/*
 * Alternative scrape approach, grab nodeValue lines from whole section.
 */
function _stockapi_parse_web($value, $corp) {
  $sans_vide = preg_replace("/(^[\r\n]*|[\r\n]+)[\s\t]*[\r\n]+/", "\n", $value);
  //watchdog('stockapi_sans_vide', '<pre>@print_r</pre>', array('@print_r' => print_r( $sans_vide, TRUE)));

  $array = explode("\n", $sans_vide);
  $stock = array();
  $stock[$corp][0]= $corp;//symbol
  foreach ($array as $key => $value) {
    $date_temp = '';
    if ($key == 1) {
      $stock[$corp][$key] = $value; // trade_price_only
    }
    if ($key == 2) {
      $stock[$corp][$key] = substr($value, (stripos($value, '+')+1)); //chg
    }
    if ($key == 3) {
      $stock[$corp][$key] = $value; //pct_chg
    }
    if ($key == 4) {
      $date_temp = substr($value, 0, stripos($value, ' -'));
      $stock[$corp][$key] = strtotime($date_temp . ' 2018'); // Updated
    }
    if (stripos($array[$key], 'Close') > 0) {
      $stock[$corp][5] = $array[1];// previous-close
    }
    if (stripos($array[$key], 'ange') == 1) {
      $stock[$corp][6] = $array[$key+1];// days_range
    }
    if (stripos($array[$key], '2 week') == 1) {
      $stock[$corp][7] = $array[$key+1];// 52_week_range
    }
    if (stripos($array[$key], 'pen') == 1) {
      $stock[$corp][8] = $array[$key+1];// open
    }
    if (stripos($array[$key], 'ol / Avg.') == 1) {
      $val = $array[$key+1];
      $stock[$corp][9] = substr($val, 0, (stripos($val, '/')));// volume
      $stock[$corp][10] = substr($val, (stripos($val, '/')+1));// avg_daily_volume
    }
    if (stripos($array[$key], 'kt cap') == 1) {
      $stock[$corp][11] = $array[$key+1];// market_cap
    }
    if (!empty($date_temp)) {
      $stock[$corp][12] = $date_temp; // trade_date string
      $stock[$corp][13] = $date_temp; // last_trade_date string
    }

  }
  return $stock;
}

function _file_get_contents($url) {

  // Load the JavaScript file

  // Trough drupal_http_request if remote
  $client = \Drupal::httpClient();
  $response = $client->request('GET', $url);
  if(preg_match('/^(http|https|proxy)\:\/\//i',$url)) {
    try {
      $page = $response->getBody();
      //$page = drupal_http_request($url)->data;
      if (!isset($page)) {
        throw new Exception('page not set');
      }
    } catch(Exception $e) {
      if (!isset($page)) {
        \Drupal::logger('stockapi')->notice('Sleep 11 seconds processing url:<pre>@print_url</pre>', array('@print_url' =>  $url));
        //watchdog('stockapi_info', 'Sleep 11 seconds processing url:<pre>@print_url</pre>',
          //array('@print_url' =>  $url),
          //WATCHDOG_NOTICE);
        sleep(11);
        $data = $response->getBody();
        //$page = drupal_http_request($url)->data;
      }
    }
    // Or through the usual way
  } else {
    $page = file_get_contents($url);
  }
  // Terms of service: https://www.alphavantage.co/terms_of_service
  sleep(4); // The api provider wishes no more than 1 request per second.
  return $page;
}
/**
 * Save a single stock quote to the database.
 *
 * @param $stock object
 *   The stock object keys' must match the field names of the database table.
 *   The stock object values' are the data to insert.
 *
 * @return
 *   TRUE on success, FALSE on failure
 */
function stockapi_save($stock) {
  // Fix potential text in float fields.
  $stock = _stockapi_fix_floats($stock);

  if (isset($stock->symbol) && db_query("SELECT symbol FROM {stockapi} WHERE symbol = :symbol", array(':symbol' => $stock->symbol))->fetchField()) {
    // Update
    $symbol = $stock->symbol;
    $stock = (array) $stock;
    unset($stock['symbol']); // Primary key

    $updated = db_update('stockapi')
        ->fields((array) $stock)
        ->condition('symbol', $symbol)
        ->execute();

    return $updated;
  }
  else {
    //Insert
    $inserted = db_insert('stockapi')
        ->fields((array) $stock)
        ->execute();

    return $inserted;
  }
}


/**
 * Return ticker information for a single symbol from our database, not Alpha Vantage.
 *
 * @param $symbol string
 *   The ticker symbol to retrieve information for.
 * @param $q string
 *   The type of information to retrieve. Can be either: basic, extended,
 *   or realtime. The default is the site-wide setting, which is usually the
 *   best bet here.
 * @return object
 */
function stockapi_load($symbol, $q = NULL) {
  static $stocks = array();

  if (!\Drupal\Component\Utility\Unicode::strlen($symbol)) {
    return array();
  }

  if (!isset($stocks[$symbol])) {
    $fields = _stockapi_get_fields($q);
    $results = db_query("SELECT " . implode(',', $fields) . " FROM {stockapi} WHERE symbol = :symbol", array(':symbol' => $symbol), array('fetch' => PDO::FETCH_ASSOC));
    if (count($results)) {
      foreach ($results as $result) {
        $stocks[$symbol] = $result;
      }
    }
  }
  else {
    $stocks[$symbol] = array();
  }

  return $stocks[$symbol];
}

/**
 * Return ticker information for a batch of symbols from our database, not Alpha Vantage.
 *
 * @param $symbol array
 *   An array of ticker symbols to retrieve information for.
 * @param $q string
 *   The type of information to retrieve. Can be either: basic, extended,
 *   or realtime. The default is the site-wide setting, which is usually the
 *   best bet here.
 * @return array
 *   An array of stocks, keyed by their ticker symbol. Each stock is an object within the array.
 */
function stockapi_multiload($symbol, $q = NULL) {
  $stocks = array();
  $fields = _stockapi_get_fields($q);
  $symbols = implode(', ', array_map('_stockapi_quote_it', $symbol));
  $results = db_query("SELECT " . implode(', ', $fields) . " FROM {stockapi} WHERE symbol IN ({$symbols})");
  foreach ($results as $stock) {
    $stocks[$stock->symbol] = $stock;
  }

  return $stocks;
}

/**
 * Wrap a string in quotes. Usually used with array_map().
 */
function _stockapi_quote_it($string, $style = 'single') {
  return ($style != 'single') ? '"' . $string . '"' : "'" . $string . "'";
}

/**
 * Return the field parameter string to pull the correct columns from Alpha Vantage.
 */
function stockapi_get_quotetype($q = NULL) {
  $quotetype = array();
  $quotetype['basic'] = 'snl1d1t1c1p2va2bapomwerr1dyj1x';
  $quotetype['extended'] = $quotetype['basic'] . 'cs7t8e7e8e9r6r7r5b4p6p5j4m3m4k3t7it6j5i5c8r2g6g5w4v7n4l2l3g3d2g4g1w1v1c3p1s1m6m5m8m7qk5k4j6';
  $quotetype['realtime'] = $quotetype['basic'] . 'b2b3k2k1c6m2j3';

  if ($quotetype[$q]) {
    return $quotetype[$q];
  }
  // Always return the basic set as a fall through.
  return $quotetype['basic'];
}

/**
 * Convert a stock from Alpha Vantage to its object oriented counterpart.
 *
 * @return object
 */
function _stockapi_to_object($stock, $q = NULL) {
  $fields = _stockapi_get_fields($q);
  $ns = count($fields);
  if ($ns) {
    $s = new stdClass();
    for ($i = 0; $i < $ns; $i++) {
      $s->{$fields[$i]} = $stock[$i];
      if (isset($stock[$i])) {
        $s->{$fields[$i]} = $stock[$i];
      }
      else {
        $s->{$fields[$i]} = NULL;
        \Drupal::logger('stockapi_info')->notice('<pre>@print_r</pre>', array('@print_r'
         => print_r( "Symbol " . $stock[0] . " Element $i NULL", TRUE)));
      }
    }
  }
  $s->updated = REQUEST_TIME;

  return $s;
}

/**
 * Given the current site-wide quotetype, generate the actual database fields
 * names currently active. These field names usually double as the keys for the
 * stock object.
 */
function _stockapi_get_fields($q = NULL) {
  if (!$q) {
    $q = \Drupal::config('stockapi.settings')->get('stockapi_quotetype');
  }
  preg_match_all('/[a-z][0-9]?/', stockapi_get_quotetype($q), $yfields);
  $yfields = $yfields[0];

  $fields = explode(', ', 'symbol, last_trade_price_only, chg, pct_chg, updated, previous_close, days_range, 52_week_range, open, volume, avg_daily_volume, market_cap, trade_date, last_trade_date, pe_ratio, float_shares, chg_and_pct_chg, last_trade_time, exchange, name');
  //watchdog('stockapi_fields_debug', '<pre>@print_r</pre>', array('@print_r' => print_r( $fields, TRUE)));
  return $fields;
}

/**
 * Internal helper function to deal cleanly with various HTTP response codes.
 */
function _stockapi_request_failure($results) {
  switch ($results->code) {
    case '200': // Success!
    case '304': // Not modified, nothing to do.
      return FALSE;
    default:
      \Drupal::logger('stockapi')->notice('Failed to retrieve stock quotes with error: %error', array('%error' => $results->error));
      return TRUE;
  }
  return FALSE;
}

/**
 * Internal helper function to change text values returned for float
 * fields to '0'.
 */
function _stockapi_fix_floats($stock) {
  $stock = (array) $stock;
  $schema = drupal_get_module_schema('stockapi');

  foreach ($schema['fields'] as $name => $field) {
    if ($field['type'] == 'float' && isset($stock[$name]) && !is_numeric($stock[$name])) {
      $stock[$name] = 0;
    }
  }

  return (object) $stock;
}
