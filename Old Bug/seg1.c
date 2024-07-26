        do_delete_account (account, NULL, NULL, NULL);

        /* update opening balance account */
        gnc_find_or_create_equity_account (gnc_account_get_root(account),
                                           EQUITY_OPENING_BALANCE,
                                           xaccAccountGetCommodity (account));