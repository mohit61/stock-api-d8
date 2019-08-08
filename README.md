## Description:

- The [Stock Api](https://www.drupal.org/project/stockapi) module is a stock quote API module that provides other modules with a facility to incorporate stock data in them.

- Stock data retrieved from Alpha Vantage Realtime historical equity data is cached locally for performance.

- Up to 42 different fields regarding a stock can be retrieved. Each field is stored as structured data in the local database (e.g., converted to a float, varchar, etc.).

- Stock information from Alpha Vantage Realtime historical equity data is automatically updated, at an admin specified interval.

- The updates for stock information are processed one symbol at a time for however many symbols, all the symbols will be processed during  a stockapi cron job.

### Installation

- Go to drupal/module directory.

- Execute the drush command `drush dl stockapi` for downloading the latest compatible version of Stock API for Drupal 8.

- Run `drush en stockapi` for installing the module.
