static void
gnc_plugin_page_account_tree_cmd_delete_account (GtkAction *action, GncPluginPageAccountTree *page)
{
    Account *account = gnc_plugin_page_account_tree_get_current_account (page);
    gchar *acct_name;
    GtkWidget *window;
    Adopters adopt;
    GList* list;
    gint response;
    GList *filter = NULL;
    GtkWidget *dialog = NULL;
    if (account == NULL)
        return;
    memset (&adopt, 0, sizeof (adopt));
    /* If the account has objects referring to it, show the list - the account can't be deleted until these
       references are dealt with. */
    list = qof_instance_get_referring_object_list(QOF_INSTANCE(account));
    if (list != NULL)
    {
#define EXPLANATION _("The list below shows objects which make use of the account which you want to delete.\nBefore you can delete it, you must either delete those objects or else modify them so they make use\nof another account")
        gnc_ui_object_references_show(EXPLANATION, list);
        g_list_free(list);
        return;
    }
    window = gnc_plugin_page_get_window(GNC_PLUGIN_PAGE(page));
    acct_name = gnc_account_get_full_name(account);
    if (!acct_name)
        acct_name = g_strdup (_("(no name)"));
    if (gnc_account_n_children(account) > 1) {
        gchar* message = g_strdup_printf("Account '%s' has more than one subaccount, move subaccounts or delete them before attempting to delete this account.", acct_name);
        gnc_error_dialog(GTK_WINDOW(window),"%s", message);
        g_free (message);
        g_free(acct_name);
        return;
    }
    // If no transaction or children just delete it.
    if (!(xaccAccountGetSplitList (account) != NULL ||
          gnc_account_n_children (account)))
    {
        do_delete_account (account, NULL, NULL, NULL);

        /* update opening balance account */
        gnc_find_or_create_equity_account (gnc_account_get_root(account),
                                           EQUITY_OPENING_BALANCE,
                                           xaccAccountGetCommodity (account));
        return;
    }

    dialog = account_delete_dialog (account, GTK_WINDOW (window), &adopt);
    while (TRUE)
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response != GTK_RESPONSE_ACCEPT)
        {
            /* Account deletion is cancelled, so clean up and return. */
            filter = g_object_get_data (G_OBJECT (dialog),
                                        DELETE_DIALOG_FILTER);
            gtk_widget_destroy(dialog);
            g_list_free(filter);
            return;
        }
        adopter_set_account_and_match (&adopt.trans);
        adopter_set_account_and_match (&adopt.subacct);
        adopter_set_account_and_match (&adopt.subtrans);
        if (adopter_match (&adopt.trans, GTK_WINDOW (window)) &&
@@ -1709,13 +1708,12 @@
                                adopt.subacct.new_account,
                                adopt.delete_res) == GTK_RESPONSE_ACCEPT)
    {
        do_delete_account (account, adopt.subacct.new_account,
                           adopt.subtrans.new_account, adopt.trans.new_account);

        /* update opening balance account */
        gnc_find_or_create_equity_account (gnc_account_get_root(account),
                                           EQUITY_OPENING_BALANCE,
                                           xaccAccountGetCommodity (account));
       
    }
}